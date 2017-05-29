#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "usage.h"

volatile int quit = 0;

void signalHandler(int signal) {
	(void) signal;
	quit = 1;
}

int main(const int argc, const char **argv)
{
	if(print_usage(argc, argv))
		return 1;

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	// TODO start jobs...

	while(!quit)
		pause();

	// TODO terminate jobs...

	printf("For clients: %ld:%d\n"
			"For promoters: %ld:%d\n",
			getClientBindAddress(), getClientBindPort(),
			getPromoterBindAddress(), getPromoterBindPort());
}
