#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
static void handler(int sig, siginfo_t *si, void *unused)
{
    printf("Got SIGSEGV at address: 0x%lx\n",(long) si->si_addr);
    printf("Bailing out!");
    exit(1);
}

int main(int argc, char *argv[])
{
    char *p; char a;
    int pagesize;
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
      printf("Failure creating handler");

    int *bad = (int *)0xfffffffffff;
    printf("val is: %d", *bad);

    // Shouldn't get here, because we just page faulted
    printf("How did we get here!!!!?");
}

