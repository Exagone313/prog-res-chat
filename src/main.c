#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include "constants.h"
#include "state.h"
#include "thread.h"
#include "usage.h"

volatile int quit = 0;

void signalHandler(int signal) {
	(void) signal;
	quit = 1;
}

int main(const int argc, const char **argv)
{
	m_state state;
	pthread_t master_thread;
	int ret;

	ret = 1;

	if(print_usage(argc, argv))
		goto exit;

	if(getuid() == 0 || getgid() == 0) {
		err("This program must not be ran as root.");
		goto exit;
	}

	dbgf("m_state size: %zu\n", sizeof(state));

	signal(SIGINT, signalHandler);
	signal(SIGTERM, signalHandler);

	if(pipe(state.net.exitpipe) < 0)
		goto exit;
	if(pthread_attr_init(&state.thread_attr))
		goto exit;
	pthread_attr_setdetachstate(&state.thread_attr, PTHREAD_CREATE_JOINABLE);

	state_init(&state);
	state.main = pthread_self();
	if(pthread_create(&master_thread, &state.thread_attr,
				master_thread_func, (void *) &state))
		goto free;

	ret = 0;
	while(!quit)
		pause();

	state.quit = 1;
	close(state.net.exitpipe[1]);
	pthread_join(master_thread, NULL);

free:
	pthread_attr_destroy(&state.thread_attr);
exit:
	return ret;
}
