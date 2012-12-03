#include "sic-internals.h"

/** Global client state **/

// Client ID for this client
client_id sic_client_id = -1;

// Base range of this clients shared memory mappings
void * shared_base;
void * next_free;

// Whether this client is blocked on a barrier
bool blocked;

// List of invalid pages
static PageInfo *invalid_pages;

/** Init / Exit **/

void initialize_client() {
  // MMAP the shared block of memory
  shared_base = mmap(NULL, SHARED_SIZE,
                     PROT_READ,
                     MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (shared_base == MAP_FAILED)
    sic_panic("Could not map shared memory space. Out of VA space.");

  next_free = shared_base;

  // Request an id from the server and wait for the response
  wait_for_server();
  // Initialize our internal memory manager
  initialize_memory_manager();
}

void cleanup_client() {
  munmap(shared_base, SHARED_SIZE);
}

// Currently a very stupid alloc ...
void * alloc(size_t len) {
  void * ret = next_free;
  len = ROUNDUP(len, PGSIZE);
  if ((next_free + len) - shared_base > SHARED_SIZE)
    ret = NULL;
  else
    next_free = ret + len;
  return ret;
}

/** Synchronization **/

int signal_server(message_t code, int value, message_t expected_ack) {
  uint8_t msg[MSGMAX_SIZE];
  int len = encode_message(msg, sic_id(), code, value);
  return send_message_to_server(msg, len, expected_ack);
}

int send_message_to_server(uint8_t *msg, int len, message_t expected_ack) {
  int sid, scode, svalue;
  uint8_t resp[256];
  sic_logf("Sending all the signals to the server\n");
  send_message(SERVER_IP, SERVER_PORT, msg, len, resp);
  decode_message(resp, &sid, &scode, &svalue);
  sic_logf("Got response code from server: %d\n", scode);
  if (expected_ack && scode != expected_ack)
    sic_panic("Networking error, could not get ack from server");
  return scode;
}

void arrived_at_barrier(barrier_id id) {
  blocked = true;
  sic_logf("[CLIENT] %d hitting barrier: %d\n", sic_id(), id);

  // Compute memory diff to send
  uint8_t msg[MSGMAX_SIZE];
  memset(msg, 0, MSGMAX_SIZE);
  int len = diff_and_cleanup(msg, sic_id(), CLIENT_AT_BARRIER, id);
  send_message_to_server(msg, len, ACK_CLIENT_AT_BARRIER);
  //signal_server(CLIENT_AT_BARRIER, id, ACK_CLIENT_AT_BARRIER);
  while(blocked) {
    sched_yield();
  }
}

void released_from_barrier(barrier_id id) {
  assert(!blocked);
  blocked = false;
  sic_logf("[CLIENT] %d released from barrier %d",sic_id(), id);
}

void acquire_lock(lock_id lock) {
 while(signal_server(CLIENT_REQUEST_LOCK, lock, NO_ACK) == SERVER_LOCK_NOT_ACQUIRED) {
   sched_yield();
 }
 sic_logf("[CLIENT] %d acquired lock %d", sic_id(), lock);
}

void release_lock(lock_id lock) {
 int code = signal_server(CLIENT_RELEASE_LOCK, lock, NO_ACK);
 if (code == SERVER_LOCK_NOT_RELEASED) {
   sic_logf("[CLIENT] Could not release lock %d because %d never acquired it", lock, sic_id());
 } else {
   sic_logf("[CLIENT] Released lock %d held by %d", lock, sic_id());
 }
 // diff and cleanup!?
}

/** Networking **/

void wait_for_server() {
  sic_client_id = 0;
  uint8_t msg[MSGMAX_SIZE], resp[256];
  int len = encode_message(msg, -1, CLIENT_INIT, 0);
  send_packet(SERVER_IP, SERVER_PORT, msg, len, resp);
  int sid, scode, svalue;
  decode_message(resp, &sid, &scode, &svalue);
  sic_client_id = svalue;
  sic_logf("[CLIENT] Initialized with id: %d\n", sic_id());
}

int sic_id() {
  return sic_client_id;
}

void *runclient(void * args) {
  int sid, scode, svalue;
  int listener_d = open_listener_socket();
  int port = CLIENT_BASE_PORT + sic_id();
  sic_logf("Client %d beginning to listen on port %d\n", sic_id(), port);
  uint8_t buffer[256];
  bind_to_port(listener_d, port); 
  listen(listener_d, 10);
  while (1) {
    struct sockaddr_storage server_addr;
    unsigned int address_size = sizeof(server_addr);
    int connect_d = accept(listener_d, (struct sockaddr*) &server_addr, &address_size);

    // Receive the message from the server
    memset(buffer, 0, sizeof(buffer));
    recv_data(connect_d, buffer, 255);

    // Process the message
    decode_message(buffer, &sid, &scode, &svalue);
    uint8_t msg[MSGMAX_SIZE];
    int len = dispatch(msg, sid, scode, svalue);

    // Send back the response message
    send(connect_d, msg, len, 0);
    close(connect_d);
  }
}

int dispatch(uint8_t* return_msg, int id, int code, int value) {
  sic_logf("Client processing: %d %d %d\n", id, code, value);
  switch (code) {
    case SERVER_RELEASE_BARRIER:
      released_from_barrier((barrier_id) value);
      return encode_message(return_msg, sic_id(), ACK_RELEASE_BARRIER, value);
      break;
    default:
      sic_logf("Can't handle code: %d\n", code);
      return -1;
  }
}

/** Memory Management **/

static void handler(int sig, siginfo_t *si, void *unused) {
  sic_logf("Got SIGSEGV at address: 0x%lx",(long) si->si_addr);
  sic_logf("Marking it writeable");
  void * failing_page = ROUNDDOWN(si->si_addr, PGSIZE);
  if(mprotect(failing_page, PGSIZE, PROT_READ | PROT_WRITE) < 0) {
    printf("mprotect failed.");
  }
  sic_logf("Cloning page");
  register_page(failing_page, twin_page(failing_page));
}

void initialize_memory_manager() {
  // Setup sigaction handler
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    printf("Failure creating handler\n");

}

void mark_read_only(void *start, size_t length) {
  mprotect(start, length, PROT_READ);
}

void *twin_page(void *va) {
  void *new_page = memalign(PGSIZE, PGSIZE);
  memcpy(new_page, va, PGSIZE);
  return new_page;
}

/** Appends to the head of the list of invalid pages */
void register_page(void *realva, void *twinnedva) {
  if (invalid_pages == NULL) {
    invalid_pages = (PageInfo *)malloc(sizeof(PageInfo));
  } else { 
    PageInfo *new = (PageInfo *)malloc(sizeof(PageInfo));
    new->next = invalid_pages;
    invalid_pages = new;
  }
  invalid_pages->real_page_addr = realva;
  invalid_pages->twinned_page_addr = twinnedva;
  invalid_pages->diff.num_diffs = -1;
  
  invalid_pages->next = NULL;
  sic_logf("Registered %p cloned at %p", realva, twinnedva);
}

/* 
 * Computes the diffs, frees the locally mapped pages, and marks the
 * real page read only again.
 */
int diff_and_cleanup(uint8_t *msg, client_id client, int code, int value) {
  sic_logf("About to create diffs");
  Transmission t = TRANSMISSION__INIT;
  t.id = client;
  t.code = code;
  t.value = value;

  memstat();
  PageInfo *w = invalid_pages;
  int num_pages = 0;
  while(w) {
    w->diff = diff_for_page(w);
    free(w->twinned_page_addr);
    mark_read_only(w->real_page_addr, PGSIZE);
    w = w->next;
    num_pages++;
  }
  
  RegionDiffProto **pages = malloc(num_pages * sizeof(RegionDiffProto *));
  w = invalid_pages;
  // For each diff, we want a RegionDiffProto
  int i = 0;
  while(w) {
    RegionDiffProto *r = malloc(sizeof(RegionDiffProto));
    RegionDiffProto tmp = REGION_DIFF_PROTO__INIT;
    *r = tmp;
    to_proto(w->diff, r);
    // We're done with the region diff
    pages[i] = r;
    w = w->next;
    i++;
  }
  t.diff_info = pages;
  t.n_diff_info = i;
  int len = encode_transmission(msg, &t);
  sic_logf("diff encoded size: %u", transmission__get_packed_size(&t));
  for ( i = 0; i < t.n_diff_info; i++ ) {
    printf("freed: %p\n", *pages[i]->diffs);
    free(*pages[i]->diffs);
    free(pages[i]->diffs);
    free(pages[i]);
  }
  free(pages);
  return len;
}

void to_proto(RegionDiff r, RegionDiffProto *rp) {
  rp->diffs = (DiffSegment **)malloc(r.num_diffs * sizeof(DiffSegment *));  
  int i;
  for(i = 0; i < r.num_diffs; i++) {
    rp->diffs[i] = &r.diffs[i];
  }
  rp->n_diffs = r.num_diffs;
}

RegionDiff diff_for_page(PageInfo *page) {
  return memdiff(page->twinned_page_addr, page->real_page_addr, PGSIZE);
}

/** Logs the current state off memory affairs **/
void memstat() {
  sic_logf(" ----- Computing memstat ---- ");
  PageInfo *w = invalid_pages;
  int num_pages = 0;
  while(w) {
    num_pages++; 
    w = w->next;
  }
  sic_logf("Num cloned pages: %d", num_pages);
  w = invalid_pages;
  while(w) {
    sic_logf("about to make diff");
    // The "real" page contains the new content
    RegionDiff diff = diff_for_page(w);
    sic_logf("about to print");
    print_diff(diff);
    free(diff.diffs);
    w = w->next;
  }
}
