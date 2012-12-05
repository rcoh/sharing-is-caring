#include "sic-util.h"
#include "time.h"
#include "assert.h"

void sic_panic(char * msg) {
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(1);
}

void sic_log(const char* msg) {
  sic_log_fn(NULL, msg);
}

void sic_logf(const char *fmt, ...) {
  va_list args;
  va_start(args,fmt);
  fprintf(stderr, "LOG: ");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
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
  buf += 4;
  sic_logf("Encoded size: %u", transmission__get_packed_size(trans));
  int len = transmission__pack(trans, buf);
  buf[len] = '\0';
  buf -= 4;
  *buf = len;
  return len + 4;
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

Transmission *decode_transmission(uint8_t *msg) {
  int len = (size_t) *msg;
  msg += 4;
  Transmission *trans;
  // Unpack the message using protobuf-c.
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
  printf("alloced: %p\n", diffs);
  
  RegionDiff r;
  r.diffs = diffs;
  r.num_diffs = num_diffs;
  return r;
}

void apply_diff(void *page_addr, RegionDiff diff) {
  DiffGranularity *w = page_addr;
  int i;
  for (i = 0; i < diff.num_diffs; i++) {
    w += diff.diffs[i].length;
    *w = diff.diffs[i].new_data;
  }
}

RegionDiff merge_diffs(int num_diffs, RegionDiff *r) {
  // TODO: probably shouldn't hardcode PGSIZE here
  void *new_page = malloc(PGSIZE);
  memset(new_page, 0, PGSIZE);
  int i;
  for (i = 0; i < num_diffs; i++) {
    apply_diff(new_page, r[i]);
  }

  // TODO: this is inefficient...
  void *zero_page = malloc(PGSIZE);
  memset(new_page, 0, PGSIZE);

  RegionDiff res = memdiff(zero_page, new_page, PGSIZE);
  free(new_page);
  free(zero_page);
  return res;
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
  sic_logf("Printing Diff.");
  sic_logf("\tDiff contains %d components", diff.num_diffs);
  sic_logf("\tDiff contents: ");
  DiffSegment *cur = diff.diffs;
  int num_diffs = 0;
  while(num_diffs < diff.num_diffs) {
    sic_logf("\t\t%d Unmodified word -> %x.", 
        cur[num_diffs].length, cur[num_diffs].new_data);
    num_diffs++;
  }
}

const char * get_message(message_t message) {
  switch (message) {
    case CLIENT_AT_BARRIER:
      return "Client -> at barrier";
    case ACK_CLIENT_AT_BARRIER:
      return "Ack client at barrier";
    case CLIENT_INIT:
      return "Client init";
    default:
      return "Unknown";
  }
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
