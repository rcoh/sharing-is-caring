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
  vfprintf(stderr, fmt, args);
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

