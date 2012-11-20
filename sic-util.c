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

int encode_message(char* msg, int id, int code, int value) {
  assert(id < 100);
  assert(code < 100);
  assert(value < 100);
  return snprintf(msg, 10, "%02d %02d %02d\n", id, code, value);
}

int decode_message(char* msg, int* id, int* code, int* value) {
  return sscanf(msg, "%02d %02d %02d", id, code, value);
}

// Pointer to an array of 
// (length, diff)
//
RegionDiff memdiff(void *old, void *new, size_t length) {
  const uint8_t * op = old;
  const uint8_t * np = new;
  size_t run_length = 0;
  size_t bytes_consumed = 0;
  size_t num_diffs = 0;
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
  diffs = (MemDiff *)realloc((void *)diffs, num_diffs * sizeof(MemDiff));

  RegionDiff r;
  r.diffs = diffs;
  r.num_diffs = num_diffs;
  return r;
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
