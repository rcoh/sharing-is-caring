#include <sys/mman.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>

#define PGSIZE 4096
// Rounding operations (efficient when n is a power of 2)
// Round down to the nearest multiple of n
#define ROUNDDOWN(a, n)           \
({                \
  uintptr_t __a = (uintptr_t) (a);        \
  (typeof(a)) (__a - __a % (n));        \
})
// Round up to the nearest multiple of n
#define ROUNDUP(a, n)           \
({                \
  uintptr_t __n = (uintptr_t) (n);        \
  (typeof(a)) (ROUNDDOWN((uintptr_t) (a) + __n - 1, __n)); \
})

static void handler(int sig, siginfo_t *si, void *unused)
{
  printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
  printf("Marking it writeable\n");
  void * failing_page = ROUNDDOWN(si->si_addr, PGSIZE);
  if(mprotect(failing_page, PGSIZE, PROT_READ | PROT_WRITE) < 0) {
    printf("mprotect failed.");
  }
}

int main(int argc, char *argv[])
{
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    printf("Failure creating handler");

  // Exactly one page
  char * buffer = memalign(PGSIZE, PGSIZE);

  if(mprotect(buffer, PGSIZE, PROT_READ) < 0) {
    printf("Memprotect failure");
  }
  buffer[0] = 'a';
  buffer[1] = 'b';
  buffer[2] = '\0';
  printf("Buffer [should be 'ab']: %s", buffer);
  return 0;
}

