#include <sys/mman.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/ptrace.h>



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

int main(int argc, char *argv[], char *envp[])
{
  struct sigaction sa;

  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = handler;
  if (sigaction(SIGSEGV, &sa, NULL) == -1)
    printf("Failure creating handler");

  // Exactly one page
  char **new_argv = malloc(sizeof(char *) * (argc + 1));
  int i;
  for(i = 0; i < argc; i++) {
    new_argv[i] = argv[i];
  }
  char * buffer = memalign(PGSIZE, PGSIZE);
  buffer[0] = '/';
  buffer[1] = 'e';
  buffer[2] = 't';
  buffer[3] = 'c';
  buffer[4] = '\0';

  new_argv[2] = buffer;
  new_argv[3] = '\0';
  // argv  = [excpm, ls, /etc]
  if(mprotect(buffer, PGSIZE, PROT_NONE) < 0) {
    printf("Memprotect failure");
  }
  
  int r = fork();
  if(r == 0) {
    printf("in child, running exec: %s\n", new_argv[1]);
//    printf("arg is: %s\n", new_argv[2]);
    int x = execve(new_argv[1], new_argv + 1, envp);
    printf("exec returned: %d", x);
  } else {
    printf("in parent, peacing out\n");
    return 0;
  }
  /*buffer[0] = 'a';
  buffer[1] = 'b';
  buffer[2] = '\0';
  printf("Buffer [should be 'ab']: %s. We are: %d", buffer, r);*/
  return 0;
}

