#define _GNU_SOURCE

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "constants.h"
#include "state.h"
#include "thread.h"

void *master_thread_func(void *cls) // master thread
{
	m_state *state;
	pthread_t net_thread;
	pthread_t unit_thread[THREAD_POOL_UNITS];
	int err, thread_i, exitpipe[2];

	err = 1;
	thread_i = 0;
	state = (m_state *) cls;

	if(pthread_mutex_init(&state->comm_mutex, NULL))
		goto exit;
	if(pthread_mutex_init(&state->pool_mutex, NULL))
		goto destroy_comm_mutex;
	if(pthread_cond_init(&state->comm_cond, NULL))
		goto destroy_pool_mutex;
	if(pthread_cond_init(&state->pool_cond, NULL))
		goto destroy_comm_cond;
	if(pipe(exitpipe) < 0)
		goto clean;

	state->net.exitpipe = exitpipe[0];
	state->net.connected = -1;

	pthread_mutex_lock(&state->comm_mutex);

	dbg("m create net");
	if(pthread_create(&net_thread, &state->thread_attr,
				net_thread_func, (void *) state)) {
		pthread_mutex_unlock(&state->comm_mutex);
		goto clean;
	}

	do { // block until net thread creates socket(s)
		dbg("m wait");
		pthread_cond_wait(&state->comm_cond, &state->comm_mutex);
		dbg("m signaled");
	} while(state->net.connected == -1);

	if(state->net.connected == -2) { // net thread failed to start networking
		pthread_mutex_unlock(&state->comm_mutex);
		goto quit;
	}

	dbg("m create units");
	// start thread pool units
	for(; thread_i < THREAD_POOL_UNITS; thread_i++) {
		if(pthread_create(&unit_thread[thread_i], &state->thread_attr,
					unit_thread_func, (void *) (state->unit + thread_i)))
			goto quit;
	}
	dbg("m unlock");
	pthread_mutex_unlock(&state->comm_mutex);

	dbg("m successful start");
	err = 0;

	// TODO loop, block on communication
	sleep(10);
quit:
	state->quit = 1;

	// stop net thread
	close(exitpipe[1]);
	pthread_join(net_thread, NULL);

	// stop units
	pthread_cond_broadcast(&state->comm_cond);
	pthread_cond_broadcast(&state->pool_cond); // FIXME
	for(thread_i--; thread_i >= 0; thread_i--) {
		dbgf("Join thread %d.", thread_i);
		pthread_join(unit_thread[thread_i], NULL);
	}
clean:
//destroy_pool_cond:
	pthread_cond_destroy(&state->pool_cond);
destroy_comm_cond:
	pthread_cond_destroy(&state->comm_cond);
destroy_pool_mutex:
	pthread_mutex_destroy(&state->pool_mutex);
destroy_comm_mutex:
	pthread_mutex_destroy(&state->comm_mutex);
exit:
	if(err)
		err("Failed to start server.");
	pthread_kill(state->main, SIGTERM);
	dbg("Stopping master thread.");
	pthread_exit(NULL);
}

static void create_fds(m_state *state, fd_set *readfds, fd_set *writefds)
{
	int i;

	FD_ZERO(readfds);
	FD_ZERO(writefds);

	// add exit pipe
	FD_SET(state->net.exitpipe, readfds);

	// add server sockets
	FD_SET(state->net.client_server, readfds);

	for(i = 0; i < MAX_CLIENTS; i++) {
		// add client sockets that wait for read
		if(state->net.task[i] == 1)
			FD_SET(state->net.client[i], readfds);
		// add client sockets that wait for send
		else if(state->net.task[i] == 2)
			FD_SET(state->net.client[i], writefds);
		// close socket
		else if(state->net.task[i] == 3) {
			close(state->net.client[i]);
			state->net.connected--;
			state->net.client[i] = 0;
			state->net.task[i] = 0;
		}
		// else 0 for closed/unset socket
	}

	// TODO watch promoter sockets
}

static int message_separator(const char *buffer, int buffer_length) {
	int buffer_pos, separator_pos;

	buffer_length -= 3;
	for(buffer_pos = 0; buffer_pos <= buffer_length; buffer_pos++) {
		for(separator_pos = 0; separator_pos < 3; separator_pos++) {
			if(buffer[buffer_pos + separator_pos] != '+')
				break;
		}
		if(separator_pos == 3)
			return buffer_pos;
	}

	return -1;
}

static void select_result(m_state *state, fd_set *readfds, fd_set *writefds)
{
	int i, sock, nbytes, r;
	struct sockaddr_in sin;
	socklen_t sinlen;

	// activity on connected clients
	for(i = 0; i < MAX_CLIENTS; i++) {
		// send message (highest priority)
		if(state->net.task[i] == 2 && FD_ISSET(state->net.client[i], writefds)) { // TODO
			nbytes = 0;
			//nbytes = write(state->net.client[i], state->net.buffer[i], state->net.buffer_length[i]); // FIXME need a second buffer!!! <--------
			if(nbytes == 0)
				state->net.task[i] = 3;
			else if(nbytes < 0) {
				if(errno != EAGAIN)
					state->net.task[i] = 3;
			}
			else {
				state->net.buffer_length[i] = 0; // FIXME
				state->net.task[i] = 1;
			}
		}
		// read message
		else if(state->net.task[i] == 1 && FD_ISSET(state->net.client[i], readfds)) {
			nbytes = read(state->net.client[i],
					state->net.buffer[i] + state->net.buffer_length[i],
					SOCKET_BUFFER_MAX_LENGTH - state->net.buffer_length[i]);
			state->net.buffer_length[i] += nbytes;
			if(nbytes == 0) // client closed socket
				state->net.task[i] = 3;
			else if(nbytes < 0) { // check for blocking, close socket on real error
				if(errno == EAGAIN)
					state->net.buffer_length[i]++;
				else
					state->net.task[i] = 3;
			}
			else if(0) { // check if the buffer contains the message separator
				r = message_separator(state->net.buffer[i], state->net.buffer_length[i]);
				if(r >= 0) { // TODO create read task with save separator position
				}
			}
			else if(state->net.buffer_length[i] == SOCKET_BUFFER_MAX_LENGTH) // didn't get a message with all these chars, close the socket
				state->net.task[i] = 3;
		}
	}

	// new client
	if(FD_ISSET(state->net.client_server, readfds)) {
		sinlen = sizeof(sin);
		sock = accept4(state->net.client_server,
				(struct sockaddr *) &sin, &sinlen, SOCK_NONBLOCK);
		if(sock > 0) {
			for(i = 0; i < MAX_CLIENTS; i++) {
				if(state->net.client[i] == 0) {
					state->net.client[i] = sock;
					state->net.task[i] = 1;
					state->net.connected++;
					break;
				}
			}
			if(i == MAX_CLIENTS) // cannot manage more clients
				close(sock);
		}
	}

	// writefds
}

void *net_thread_func(void *cls) // net thread
{
	m_state *state;
	int err, i;
	fd_set readfds, writefds;

	err = 1;

	state = (m_state *) cls;
	pthread_mutex_lock(&state->comm_mutex);
	dbg("net locked");

	state->net.client_server = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(state->net.client_server < 0)
		goto clean;

	// TODO bind, listen
	// TODO create promoter socket

	err = 0;

	state->net.connected = 0; // master thread waits for this value to change
	dbg("net signal");
	pthread_cond_signal(&state->comm_cond);
	dbg("net unlock");
	pthread_mutex_unlock(&state->comm_mutex);

	do {
		create_fds(state, &readfds, &writefds); // closes sockets when needed
		select(FD_SETSIZE, &readfds, &writefds, NULL, NULL); // shouldn't have any error
		if(state->quit) // TODO send messages before closing sockets
			break;
		select_result(state, &readfds, &writefds);
		// TODO create task and cond_signal unit
	} while(1);

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(state->net.client[i] > 0)
			close(state->net.client[i]);
	}

clean:
	if(err) {
		state->net.connected = -2;
		pthread_cond_signal(&state->comm_cond);
		pthread_mutex_unlock(&state->comm_mutex);
	}
	dbg("Stopping net thread.");
	pthread_exit(NULL);
}

void *unit_thread_func(void *cls) // thread pool unit
{
	u_state *unit_state;
	m_state *state;

	unit_state = (u_state *) cls;
	state = unit_state->master;

	do {
		dbgf("unit lock %d.", unit_state->id);
		if(pthread_mutex_lock(&state->comm_mutex)) {
			perror("can't lock");
			goto exit;
		}
		do {
			dbgf("unit wait %d.", unit_state->id);
			pthread_cond_wait(&state->comm_cond, &state->comm_mutex);
			if(state->quit) {
				pthread_mutex_unlock(&state->comm_mutex);
				goto exit;
			}
		} while(0); // TODO check for message
		dbgf("unit unlock %d.", unit_state->id);
		pthread_mutex_unlock(&state->comm_mutex);
	} while(1);

exit:
	dbgf("Stopping unit thread %d.", unit_state->id);
	pthread_exit(NULL);
}
