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
virt_addr VIRT(phys_addr addr) {
  return (virt_addr) ((uintptr_t)addr - (uintptr_t)shared_base);
}

phys_addr PHYS(virt_addr addr) {
  return (phys_addr) ((uintptr_t)addr + (uintptr_t)shared_base);
}

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
phys_addr alloc(size_t len) {
  void * ret = next_free;
  len = ROUNDUP(len, PGSIZE);
  if ((next_free + len) - shared_base > SHARED_SIZE)
    ret = NULL;
  else
    next_free = ret + len;
  return ret;
}

/** Synchronization **/

int signal_server(message_t code, value_t value, message_t expected_ack) {
  uint8_t msg[MSGMAX_SIZE];
  int len = encode_message(msg, sic_id(), code, value);
  return send_message_to_server(msg, len, expected_ack);
}

value_t query_server(message_t code, value_t value) {
  uint8_t msg[MSGMAX_SIZE];
  int len = encode_message(msg, sic_id(), code, value);
  int sid, scode;
  value_t svalue;
  uint8_t resp[256];
  sic_logf("Sending all the signals to the server");
  send_message(SERVER_IP, SERVER_PORT, msg, len, resp);
  decode_message(resp, &sid, &scode, &svalue);
  sic_logf("Got response code from server: %d", scode);
  return svalue;
}

int send_message_to_server(uint8_t *msg, int len, message_t expected_ack) {
  int sid, scode;
  value_t svalue;
  uint8_t resp[256];
  sic_logf("Sending all the signals to the server");
  send_message(SERVER_IP, SERVER_PORT, msg, len, resp);
  decode_message(resp, &sid, &scode, &svalue);
  sic_logf("Got response code from server: %d", scode);
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

/*void block_until_released(uint8_t *msg, int len) {
  blocked = true;
  send_message_to_server(msg, len, ACK_CLIENT_AT_BARRIER);
  //signal_server(CLIENT_AT_BARRIER, id, ACK_CLIENT_AT_BARRIER);
  while(blocked) {
    sched_yield();
  }
}*/

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
  int sid, scode;
  value_t svalue;
  decode_message(resp, &sid, &scode, &svalue);
  sic_client_id = svalue;
  sic_logf("[CLIENT] Initialized with id: %d\n", sic_id());
}

int sic_id() {
  return sic_client_id;
}

void *runclient(void * args) {
  int sid, scode;
  value_t svalue;
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

int dispatch(uint8_t* return_msg, int id, int code, value_t value) {
  sic_logf("Client processing: %d %d %d\n", id, code, value);
  switch (code) {
    case SERVER_RELEASE_BARRIER:
      released_from_barrier((barrier_id) value);
      return encode_message(return_msg, sic_id(), ACK_RELEASE_BARRIER, value);
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
  register_page(VIRT(failing_page), twin_page(failing_page));
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
void register_page(virt_addr realva, phys_addr twinnedva) {
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
int diff_and_cleanup(uint8_t *msg, client_id client, int code, value_t value) {
  sic_logf("About to create diffs");

  PageInfo *w = invalid_pages;
  int num_pages = 0;
  while(w) {
    w->diff = diff_for_page(w);
    free(w->twinned_page_addr);
    mark_read_only(w->real_page_addr, PGSIZE);
    w = w->next;
    num_pages++;
  }
  print_memstat(invalid_pages);
  sic_logf("Packaging current diff to send to server");
  return package_pageinfo(msg, client, code, value, invalid_pages);
}

RegionDiff diff_for_page(PageInfo *page) {
  return memdiff(page->twinned_page_addr, PHYS(page->real_page_addr), PGSIZE);
}


