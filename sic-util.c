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

int encode_message(uint8_t* msg, int id, int code, int value) {
  assert(id < 100);
  assert(code < 100);
  assert(value < 100);
  
  Transmission transmission = TRANSMISSION__INIT;
  transmission.id = id;
  transmission.code = code;
  transmission.value = value;
  sic_logf("Encoded size: %u", transmission__get_packed_size(&transmission));

  msg += 4;
  int len = transmission__pack(&transmission, msg);
  msg[len] = 0;
  msg -= 4;
  *msg = len;
  return len + 4;
}

int decode_message(uint8_t* msg, int* id, int* code, int* value) {
  // find the length. need to refactor this.
  int len = (size_t) *msg;
  msg += 4;
  Transmission *trans;
  // Unpack the message using protobuf-c.
  trans = transmission__unpack(NULL, len, msg);   
  if (trans == NULL)
  {
    fprintf(stderr, "error unpacking incoming message\n");
    exit(1);
  }

  *id = trans->id;
  *code = trans->code;
  *value = trans->value;
  return 0;
}

// Pointer to an array of 
// (length, diff)
//
RegionDiff memdiff(void *old, void *new, size_t length) {
  const uint8_t * op = old;
  const uint8_t * np = new;
  size_t run_length = 0, bytes_consumed = 0, num_diffs = 0;

  // Allocate the most space we could possibly need
  MemDiff *diffs = (MemDiff *)malloc(length * sizeof(MemDiff));

  while(bytes_consumed < length) {
    if (*op == *np) {
      run_length++;
    } else {
      diffs[num_diffs].length = run_length;
      diffs[num_diffs].new_content = *np;
      run_length = 0;
      num_diffs++;
    }
    bytes_consumed++;
    op++;
    np++;
  }

  // Shrink it to what we actually need
  diffs = (MemDiff *)realloc((void *)diffs, num_diffs * sizeof(MemDiff));
  
  RegionDiff r;
  r.diffs = diffs;
  r.num_diffs = num_diffs;
  return r;
}

void apply_diff(void *page_addr, RegionDiff diff) {
  uint8_t *w = page_addr;
  int i;
  for (i = 0; i < diff.num_diffs; i++) {
    w += diff.diffs[i].length;
    *w = diff.diffs[i].new_content;
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

  return memdiff(zero_page, new_page, PGSIZE);
}

void print_diff(RegionDiff diff) {
  sic_logf("Printing Diff.");
  sic_logf("\tDiff contains %d components", diff.num_diffs);
  sic_logf("\tDiff contents: ");
  MemDiff *cur = diff.diffs;
  int num_diffs = 0;
  while(num_diffs < diff.num_diffs) {
    sic_logf("\t\t%d Unmodified bytes -> %x.", 
        cur[num_diffs].length, cur[num_diffs].new_content);
    num_diffs++;
  }
}
