#include <unistd.h>
#include <stdio.h>
#include "./sic.h"

int main() {
  printf("----------------------------------------\n");
  printf("Should see child 0, parent 0, parent 1, child 1\n");
  
  if (fork() == 0) {
    sic_init();
    printf("In Child\n");

    sic_lock(0);
    printf("[CHILD] Acquired Lock 0, sleeping 1\n");
    sleep(3); // So that the parent fails to acquire the lock on 0
    sic_unlock(0);
    printf("[CHILD] Released Lock 0, sleeping 5\n");

    sleep(5);
    sic_lock(1);
    printf("[CHILD] Acquired Lock 1, sleeping 1\n");
    sic_unlock(1);
    printf("[CHILD] Released Lock 1\n");

    printf("Done with all locks. Child Done\n");
  } else {
    sleep(1); // So that the parent always starts second
    sic_init();
    printf("In Parent\n");

    sic_lock(0);
    printf("[PARENT] Acquired Lock 0, sleeping 1\n");
    sleep(1);
    sic_unlock(0);
    printf("[PARENT] Released Lock 0\n");

    sic_lock(1);
    printf("[PARENT] Acquired Lock 1\n");
    sic_unlock(1);
    printf("[PARENT] Released Lock 1\n");
    printf("Done with all locks. Parent Done\n");
  }
  sic_exit();
  return 0;
}

