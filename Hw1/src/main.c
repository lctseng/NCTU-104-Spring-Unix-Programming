#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

extern void random_length_task();

int main() {
	int n = 10;
	srand(time(0) ^ getpid());
	while(n-- > 0) {
		random_length_task();
	}
	return 0;
}

