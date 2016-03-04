#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

extern void random_length_task();

inline void add_tv(struct timeval* tv,long usec);

int main(int argc,char** argv) {
  if(argc < 2){
    printf("No number specified.\n");
    exit(-1);
  }
  int n = atoi(argv[1]);
  srand(time(0) ^ getpid());
  struct timeval target_tv,end_tv;
  gettimeofday(&target_tv, NULL);
  while(n-- > 0) {
    random_length_task();
    add_tv(&target_tv,100000);
    gettimeofday(&end_tv, NULL);
    // compute diff
    long used_usec;
    used_usec=(target_tv.tv_sec-end_tv.tv_sec)*1000000;
    used_usec+=(target_tv.tv_usec-end_tv.tv_usec);
    // printf("To sleep: %ld us\n",used_usec);
    usleep(used_usec);

  }
  return 0;
}
void add_tv(struct timeval* tv,long usec){
  tv->tv_usec += usec;
  if(tv->tv_usec >= 1000000){
    tv->tv_usec -= 1000000;
    tv->tv_sec += 1;
  }
}

/*
  // A not-so-good implementation
  struct timeval start_tv,end_tv;
  while(n-- > 0) {
    gettimeofday(&start_tv, NULL);
    random_length_task();
    gettimeofday(&end_tv, NULL);
    // compute diff
    long used_usec;
    used_usec=(end_tv.tv_sec-start_tv.tv_sec)*1000000;
    used_usec+=(end_tv.tv_usec-start_tv.tv_usec);
    usleep(100000 - used_usec);

  }


*/
