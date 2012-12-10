#include "sic-util.h"
#include "time.h"
#include "assert.h"

enum LOG_LEVEL {
  DEBUG,
  INFO
};

const int current_level = DEBUG;

void sic_panic(char * msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  fprintf(stderr, "--------------PANCIC EXITING------------");
  exit(1);
}

void sic_log(const char* msg) {
  sic_log_fn(NULL, msg);
}

void sic_debug(const char *fmt, ...) {
  if (current_level == DEBUG) { 
    va_list args;
    va_start(args,fmt);
    fprintf(stderr, "LOG: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
  }
}

void sic_info(const char *fmt, ...) {
  if (current_level <= INFO) { 
    va_list args;
    va_start(args,fmt);
    fprintf(stderr, "INFO: ");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
  }
}

void sic_log_fn(const char* fn, const char* msg) {
  FILE *file;
  if (fn != NULL)
    file = fopen(fn, "a");
  else
    file = fopen("log.txt", "a");

  time_t now;
  char buf[30];
  char *tmp;
  // Get and strip the newline out of the date format string
  time(&now);
  tmp = ctime(&now);
  strcpy(buf, tmp);
  char *p =  strchr(buf, '\n');
  if (p)
    *p = '\0';
  // Log it to the file
  fprintf(file, "[%s]: %s\n", buf, msg);
  fclose(file);
}

int encode_message(uint8_t* msg, int id, int code, value_t value) {
  memset(msg, 0, MSGMAX_SIZE);
  Transmission transmission = TRANSMISSION__INIT;
  transmission.id = id;
  transmission.code = code;
  transmission.value = value;
  return encode_transmission(msg, &transmission);
}

int encode_transmission(uint8_t *buf, Transmission *trans) {
  buf += sizeof(size_t);
  sic_debug("Encoded size: %u", transmission__get_packed_size(trans));
  int len = transmission__pack(trans, buf);
  buf[len] = '\0';
  buf -= sizeof(size_t);
  size_t* sz = (size_t*) buf;
  *sz = len;
  return len + sizeof(size_t);
}

int decode_message(uint8_t* msg, int* id, int* code, value_t* value) {
  // find the length. need to refactor this.
  Transmission *trans = decode_transmission(msg);
  *id = trans->id;
  *code = trans->code;
  *value = trans->value;
  free(trans);
  return 0;
}

Transmission* decode_transmission(uint8_t *msg) {
  int len = *((size_t*)msg);
  msg += sizeof(size_t);
  Transmission *trans;
  // Unpack the message using protobuf-c.
  sic_debug("Trying to decode a message of length %d", len);
  trans = transmission__unpack(NULL, len, msg);   
  if (trans == NULL)
    sic_panic("error unpacking incoming message\n");
  return trans;
}

// Pointer to an array of 
// (length, diff)
//
RegionDiff memdiff(void *old, void *new, size_t length) {
  const DiffGranularity * op = old;
  const DiffGranularity * np = new;
  size_t run_length = 0, bytes_consumed = 0, num_diffs = 0;

  // Allocate the most space we could possibly need
  DiffSegment *diffs = (DiffSegment *)malloc(length * sizeof(DiffSegment));

  while(bytes_consumed < length) {
    if (*op == *np) {
      run_length++;
    } else {
      // gotta hate protobufs
      DiffSegment tmp = DIFF_SEGMENT__INIT;
      diffs[num_diffs] = tmp;
      diffs[num_diffs].length = run_length;
      diffs[num_diffs].new_data = *np;
      run_length = 0;
      num_diffs++;
    }
    bytes_consumed += sizeof(DiffGranularity);
    op++;
    np++;
  }

  // Shrink it to what we actually need
  diffs = (DiffSegment *)realloc((void *)diffs, num_diffs * sizeof(DiffSegment));
  sic_debug("realloced: %p\n", diffs);
  
  RegionDiff r;
  r.diffs = diffs;
  r.num_diffs = num_diffs;
  return r;
}

void apply_diff(void *page_addr, RegionDiff diff, bool use_or) {
  DiffGranularity *w = (DiffGranularity*)page_addr;
  int i, r;
  void *failing_page = NULL;
  if(!use_or) {
    void *failing_page = ROUNDDOWN(page_addr, PGSIZE);
    if((r = mprotect(failing_page, PGSIZE, PROT_READ | PROT_WRITE)) < 0) {
      sic_debug("[ERROR mprotect failed! %s", strerror(errno));
    }
  }

  for (i = 0; i < diff.num_diffs; i++) {
    w += diff.diffs[i].length;
    if (!use_or) {
      *w = diff.diffs[i].new_data;
    } else {
      *w |= diff.diffs[i].new_data;
    }
    w++;
  }
  if(!use_or) 
    mprotect(failing_page, PGSIZE, PROT_READ);
}

RegionDiff merge_multiple_diffs(int num_diffs, RegionDiff *r) {
  // TODO: probably shouldn't hardcode PGSIZE here
  void *new_page = malloc(PGSIZE);
  memset(new_page, 0, PGSIZE);
  int i;
  for (i = 0; i < num_diffs; i++) {
    apply_diff(new_page, r[i], true);
  }

  // TODO: this is inefficient...
  void *zero_page = malloc(PGSIZE);
  memset(new_page, 0, PGSIZE);

  RegionDiff res = memdiff(zero_page, new_page, PGSIZE);
  free(new_page);
  free(zero_page);
  return res;
}

RegionDiff merge_diffs(RegionDiff r1, RegionDiff r2) {
  // TODO: probably shouldn't hardcode PGSIZE here
  void *new_page = malloc(PGSIZE);
  memset(new_page, 0, PGSIZE);
  apply_diff(new_page, r1, true);
  apply_diff(new_page, r2, true);

  // TODO: this is inefficient...
  void *zero_page = malloc(PGSIZE);
  memset(zero_page, 0, PGSIZE);

  RegionDiff res = memdiff(zero_page, new_page, PGSIZE);
  free(new_page);
  free(zero_page);
  return res;
}

/** Takes a linked list of PageInfo objects, and a set of diffs (each attached to a page).
    For each diff object it finds the matching Page (by shared address) in PageInfo creating it if
    necessary. Then it merges in the changes. Finally, it returns a pointer to the head PageInfo
**/
PageInfo* merge_multipage_diff(PageInfo* current, int n_diffinfo, RegionDiffProto **diff_info) {
  int i;
  for (i = 0; i < n_diffinfo; i++) {
    PageInfo* w = current;
    PageInfo* prev = NULL;
    RegionDiff new_diff;
    from_proto(&new_diff, diff_info[i]);
    while(w) {
      if ((uintptr_t)w->real_page_addr == diff_info[i]->start_address) {
        sic_debug("Merging changes for page %x", w->real_page_addr);
        sic_debug("Old:");
        print_diff(w->diff);
        sic_debug("New:");
        print_diff(new_diff);
        w->diff = merge_diffs(w->diff, new_diff);
        sic_debug("Resultant diff:");
        print_diff(w->diff);
        break;
        // TODO: free new_diff
      }
      prev = w;
      w = w->next;
    }

    // w is null if we go through and don't find a match
    if (w == NULL) {
      // Page not in info, add it
      sic_debug("Allocating new page @ [0x%x]", diff_info[i]->start_address);
      w = calloc(1, sizeof(PageInfo));
      if (current == NULL) {
        current = w;
      } else {
        prev->next = w;
      }
      w->real_page_addr = (virt_addr)(intptr_t)diff_info[i]->start_address;
      w->diff = new_diff;
    }
  }
  return current;
}

/** Packages up a PageInfo into a network message to send over the wire. **/
int package_pageinfo(uint8_t *msg,
                     client_id client,
                     int code,
                     value_t value,
                     PageInfo * all_pages) {
  Transmission t = TRANSMISSION__INIT;
  t.id = client;
  t.code = code;
  t.value = value;

  PageInfo *w = all_pages;
  int num_pages = 0;
  while(w) {
    w = w->next;
    num_pages++;
  }

  // TODO jolynch: Abstract this even further so we don't have
  // to construct the RegionDiffProto every time we talk to clients
  RegionDiffProto **pages = malloc(num_pages * sizeof(RegionDiffProto *));
  w = all_pages;
  // For each diff, we want a RegionDiffProto
  int i = 0;
  while(w) {
    RegionDiffProto *r = malloc(sizeof(RegionDiffProto));
    RegionDiffProto tmp = REGION_DIFF_PROTO__INIT;
    *r = tmp;
    to_proto(w->diff, r);
    r->start_address = (int64_t) (intptr_t) w->real_page_addr;
    // We're done with the region diff
    pages[i] = r;
    w = w->next;
    i++;
  }
  t.diff_info = pages;
  t.n_diff_info = i;
  int len = encode_transmission(msg, &t);
  sic_debug("diff encoded size: %u", transmission__get_packed_size(&t));
  /* TODO jolynch: stop leaking memory all over the gorram place
   * but don't segfault in server code either ... basically stop sucking */ 
  for ( i = 0; i < t.n_diff_info; i++ ) {
    //free(*pages[i]->diffs);
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

void from_proto(RegionDiff *r, RegionDiffProto *rp) {
  r->diffs = (DiffSegment *)malloc(rp->n_diffs * sizeof(DiffSegment));
  int i;
  for (i = 0; i < rp->n_diffs; i++) {
    r->diffs[i] = *rp->diffs[i];
  }
  r->num_diffs = rp->n_diffs;
}

void print_diff(RegionDiff diff) {
  sic_debug("\tDiff contains %d components", diff.num_diffs);
  sic_debug("\tDiff contents: ");
  DiffSegment *cur = diff.diffs;
  int num_diffs = 0;
  int cur_len = 0;
  while(num_diffs < diff.num_diffs) {
    sic_debug("\t\t[Loc: %d] %d Unmodified word -> \t0x%08x.", 
        cur_len, cur[num_diffs].length, cur[num_diffs].new_data);
    cur_len += (1 + cur[num_diffs].length) * sizeof(DiffGranularity);
    num_diffs++;
  }
}

/** Logs the current state off memory affairs **/
void print_memstat(PageInfo * pages) {
  sic_debug(" ----- Computing memstat ---- ");
  PageInfo *w = pages;
  int num_pages = 0;
  while(w) {
    num_pages++; 
    w = w->next;
  }
  sic_debug("Num cloned pages: %d", num_pages);
  w = pages;
  while(w) {
    sic_debug("Base address: [0x%x]", w->real_page_addr);
    print_diff(w->diff);
    w = w->next;
  }
}

const char* get_message(message_t message) {
  return message_names[message];
}

void get_transmission(char * ret, const uint8_t * msg) {
  Transmission* tran;
  tran = decode_transmission((uint8_t *)msg);
  sprintf(ret, "i: %5i, c: %10s, v: %5i",
         tran->id, get_message(tran->code), (int)tran->value);
  free(tran);
}

char * hex_repr(char * msg) {
  int i;
  char * buffer = malloc(4*strlen(msg));
  char s[4];
  memset(s, 0, sizeof(s));
  memset(buffer, 0, sizeof(buffer));
  for (i = 0; msg[i] != '\0'; i++) {
    snprintf(s, 4, " %02x ", msg[i]);
    strncat(buffer, s, 4);
  }
  return buffer;
}
