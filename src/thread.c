#define _GNU_SOURCE

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "constants.h"
#include "protocol.h"
#include "state.h"
#include "thread.h"
#include "usage.h"

void *master_thread_func(void *cls) // master thread
{
	m_state *state;
	pthread_t net_thread;
	pthread_t unit_thread[THREAD_POOL_UNITS];
	int err, thread_i, i;

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

	dbg("m successful start");
	err = 0;

	do {
		if(state->quit)
			break;

		// check if there is a message to send and unblock net thread
		for(i = 0; i < MAX_PENDING_MESSAGES; i++) {
			if(state->net.send_pending[i].socket_id >= 0) {
				dbg("m unlock net");
				write(state->net.unlockpipe[1], "m", 1);
				break;
			}
		}

		// check if there is still a message to read and broadcast pool
		for(i = 0; i < MAX_PENDING_MESSAGES; i++) {
			if(state->net.recv_pending[i].socket_id >= 0) {
				dbg("m broadcast pool");
				pthread_cond_broadcast(&state->pool_cond);
				break;
			}
		}

		dbg("m wait");
		pthread_cond_wait(&state->comm_cond, &state->comm_mutex);
		dbg("m signaled");
	} while(1);
	dbg("m unlock");
	pthread_mutex_unlock(&state->comm_mutex);

quit:
	state->quit = 1;

	// stop net thread if necessary
	write(state->net.unlockpipe[1], "q", 1);
	pthread_join(net_thread, NULL);

	// stop units
	pthread_cond_broadcast(&state->comm_cond);
	pthread_cond_broadcast(&state->pool_cond);
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
	int i, j, nbytes, has_more;

	FD_ZERO(readfds);
	FD_ZERO(writefds);

	// add exit pipe
	FD_SET(state->net.unlockpipe[0], readfds);

	// add server sockets
	FD_SET(state->net.client_server, readfds);

	for(i = 0; i < MAX_CLIENTS; i++) {
		// add client sockets that wait for send
		if(state->net.task[i] == 2) { // try to send messages until it blocks before setting writefds
			dbgf("looks like client %d has pending send messages", i);
			has_more = 0;
			for(j = 0; j < MAX_PENDING_MESSAGES; j++) {
				if(state->net.send_pending[j].socket_id != -1)
					dbgf("task %d wait for socket %d", j, state->net.send_pending[j].socket_id);
				if(state->net.send_pending[j].socket_id == i) {
					nbytes = send(state->net.client[i], state->net.send_pending[j].buffer,
							state->net.send_pending[j].buffer_length, MSG_NOSIGNAL);
					dbgf("sent task %d resulted %d to %d: %.*s (%d)", j, nbytes, i,
							state->net.send_pending[j].buffer_length, state->net.send_pending[j].buffer,
							state->net.send_pending[j].buffer_length);
					if(nbytes == 0)
						state->net.task[i] = 3;
					else if(nbytes < 0) {
						if(errno != EAGAIN)
							state->net.task[i] = 3;
						else { // will try again when it's available
							dbgf("client %d blocked (before)", i);
							has_more = 1;
							FD_SET(state->net.client[i], writefds);
							break; // stop sending messages to that client
						}
					}
					else {
						dbgf("cleared task %d", j);
						state->net.send_pending[j].socket_id = -1;
						//state->net.task[i] = 1; // FIXME there isn't only one message to send
					}
				}
			}
			if(!has_more) {
				dbgf("sent all pending messages for client %d", i);
				state->net.task[i] = 1;
			} else
				dbgf("client %d still has more messages, delayed in writefds", i);
		}
		// add client sockets that wait for read
		if(state->net.task[i] == 1)
			FD_SET(state->net.client[i], readfds);
		// close socket
		else if(state->net.task[i] == 3 && close(state->net.client[i]) == 0) {
			dbgf("disconnect client %d", i);
			state->net.connected--;
			state->net.client[i] = 0;
			state->net.task[i] = 0;
			// if the client was logged in, manually disconnect the user
			for(j = 0; j < MAX_CLIENTS; j++) { // TODO lock?
				if(state->user_socket_id[j] == i) {
					state->user_socket_id[j] = -1;
					break;
				}
			}
			// remove pending messages (both send and recv) for that user
			for(j = 0; j < MAX_PENDING_MESSAGES; j++) {
				if(state->net.send_pending[j].socket_id == i)
					state->net.send_pending[j].socket_id = -1;
				if(state->net.recv_pending[j].socket_id == i)
					state->net.recv_pending[j].socket_id = -1;
			}
		}
		// else 0 for closed/unset socket
	}

	// TODO watch promoter sockets
}

// returns the position for the first + of +++
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
	int i, j, sock, nbytes, r;
	struct sockaddr_in sin;
	socklen_t sinlen;

	// activity on unlockpipe
	if(FD_ISSET(state->net.unlockpipe[0], readfds)) {
		dbg("unlockpipe read");
		read(state->net.unlockpipe[0], &r, 1);
	}

	pthread_mutex_lock(&state->comm_mutex);

	// activity on connected clients
	for(i = 0; i < MAX_CLIENTS; i++) {
		// send message previously blocked (highest priority)
		if(state->net.task[i] == 2 && FD_ISSET(state->net.client[i], writefds)) {
			for(j = 0; j < MAX_PENDING_MESSAGES; j++) {
				if(state->net.send_pending[j].socket_id == i) {
					nbytes = send(state->net.client[i], state->net.send_pending[j].buffer,
							state->net.send_pending[j].buffer_length, MSG_NOSIGNAL);
					dbgf("sent %d to %d", nbytes, i);
					if(nbytes == 0)
						state->net.task[i] = 3;
					else if(nbytes < 0) {
						if(errno != EAGAIN)
							state->net.task[i] = 3;
						else { // will try again when it's available
							dbgf("client %d blocked (after)", i);
							break; // stop sending messages to that client
						}
					}
					else {
						state->net.send_pending[j].socket_id = -1;
						state->net.task[i] = 1;
					}
				}
			}
		}
		// read message
		else if(state->net.task[i] == 1 && FD_ISSET(state->net.client[i], readfds)) {
			// FIXME deadlock somewhere when receiving a message too long
			nbytes = read(state->net.client[i],
					state->net.buffer[i] + state->net.buffer_length[i],
					SOCKET_BUFFER_MAX_LENGTH - state->net.buffer_length[i]);
			dbgf("read %d from %d", nbytes, i);
			state->net.buffer_length[i] += nbytes;
			if(nbytes == 0) // client closed socket
				state->net.task[i] = 3;
			else if(nbytes < 0) { // check for blocking, close socket on real error
				if(errno == EAGAIN)
					state->net.buffer_length[i]++; // it was decreased by 1 before
				else
					state->net.task[i] = 3;
			}
			else { // check if the buffer contains the message separator
				do {
					r = message_separator(state->net.buffer[i], state->net.buffer_length[i]);
					if(r >= 0) {
						if(r > MESSAGE_BUFFER_MAX_LENGTH) { // disconnect client if message is too large
							dbgf("client %d sent a too large message, will close socket", i);
							state->net.task[i] = 3;
						} else {
							if(r > 0) { // create read task only for not null messages
								for(j = 0; j < MAX_PENDING_MESSAGES; j++) {
									if(state->net.recv_pending[j].socket_id == -1)
										break;
								}
								if(j == MAX_PENDING_MESSAGES) {
									dbg("overwrite first message due to full recv queue");
									j = 0;
								}
								dbgf("save message from buffer, length %d", r);
								state->net.recv_pending[j].socket_id = i;
								memcpy(state->net.recv_pending[j].buffer, state->net.buffer[i], r);
								state->net.recv_pending[j].buffer_length = r;

								// broadcast pool
								dbg("broadcast pool");
								pthread_cond_signal(&state->pool_cond);
							}
							memmove(state->net.buffer[i], state->net.buffer[i] + r + 3, state->net.buffer_length[i] - r - 3); // no need sizeof for char
							state->net.buffer_length[i] -= r + 3;
						}
					} else
						break;
				} while(1);
				if(state->net.buffer_length[i] == SOCKET_BUFFER_MAX_LENGTH) // didn't get a message with all these chars, close the socket
					state->net.task[i] = 3;
			}
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
					state->net.client_addr[i] = sin.sin_addr.s_addr;
					state->net.task[i] = 1;
					state->net.connected++;
					dbgf("Accept new client from %d:%d.", sin.sin_addr.s_addr, ntohs(sin.sin_port));
					break;
				}
			}
			if(i == MAX_CLIENTS) { // cannot manage more clients
				close(sock);
				dbg("Cannot accept a new client.");
			}
		}
	}

	pthread_mutex_unlock(&state->comm_mutex);
}

void *net_thread_func(void *cls) // net thread
{
	m_state *state;
	int err, i;
	fd_set readfds, writefds;
	struct sockaddr_in client_sin = {AF_INET, htons(getClientBindPort()), {getClientBindAddress()}, {0}};
	//struct sockaddr_in promoter_sin = {AF_INET, htons(getPromoterBindPort()), {getPromoterBindAddress()}, {0}};

	err = 1;

	state = (m_state *) cls;
	pthread_mutex_lock(&state->comm_mutex);
	dbg("net locked");

	state->net.client_server = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if(state->net.client_server < 0)
		goto clean;

	if(bind(state->net.client_server, (struct sockaddr *) &client_sin, sizeof(client_sin))) {
		perror("Cannot bind client server");
		goto close_client_server;
	}
	if(listen(state->net.client_server, 100)) {
		perror("Cannot listen client server");
		goto close_client_server;
	}

	// TODO create promoter socket

	err = 0;

	state->net.connected = 0; // master thread waits for this value to change
	dbg("net signal");
	pthread_cond_signal(&state->comm_cond);
	dbg("net unlock");
	pthread_mutex_unlock(&state->comm_mutex);

	do {
		create_fds(state, &readfds, &writefds); // closes sockets when needed
		dbg("net select");
		select(FD_SETSIZE, &readfds, &writefds, NULL, NULL); // shouldn't have any error
		if(state->quit) { // TODO send messages before closing sockets
			dbg("net got quit");
			break;
		}
		select_result(state, &readfds, &writefds);
	} while(1);

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(state->net.client[i] > 0)
			close(state->net.client[i]);
	}
close_client_server:
	close(state->net.client_server);
clean:
	if(err) {
		state->net.connected = -2;
		pthread_cond_signal(&state->comm_cond);
		pthread_mutex_unlock(&state->comm_mutex);
	} else {
		dbg("net lock");
		pthread_mutex_lock(&state->comm_mutex);
		dbg("net broadcast");
		pthread_cond_broadcast(&state->comm_cond);
		pthread_mutex_unlock(&state->comm_mutex);
	}
	dbg("Stopping net thread.");
	pthread_exit(NULL);
}

void *unit_thread_func(void *cls) // thread pool unit
{
	u_state *unit_state;
	m_state *state;
	int i, j, socket_id, s;
	char read_buffer[MESSAGE_BUFFER_MAX_LENGTH] = {0};
	int read_buffer_length;
	char send_buffer[MESSAGE_BUFFER_MAX_LENGTH + 3] = {0};
	int send_buffer_length;

	unit_state = (u_state *) cls;
	state = unit_state->master;

	dbgf("unit lock %d.", unit_state->id);
	pthread_mutex_lock(&state->comm_mutex);
	do {
		dbgf("unit locked/signaled %d.", unit_state->id);
		if(state->quit) {
			pthread_mutex_unlock(&state->comm_mutex);
			goto exit;
		}

		// check for message
		for(i = 0; i < MAX_PENDING_MESSAGES; i++) {
			if(state->net.recv_pending[i].socket_id >= 0) {
				dbgf("read message from %d: %.*s by %d", state->net.recv_pending[i].socket_id,
						state->net.recv_pending[i].buffer_length, state->net.recv_pending[i].buffer,
						unit_state->id);
				// copy message
				read_buffer_length = state->net.recv_pending[i].buffer_length;
				memcpy(read_buffer, state->net.recv_pending[i].buffer, read_buffer_length);
				socket_id = state->net.recv_pending[i].socket_id;
				state->net.recv_pending[i].socket_id = -1;
				// unlock
				dbgf("unit unlock (msg) %d.", unit_state->id);
				pthread_mutex_unlock(&state->comm_mutex);

				// TODO read message
				s = 1;
				for(j = 0; j < read_buffer_length; j++)
					send_buffer[read_buffer_length - j - 1] =
						read_buffer[j];
				send_buffer_length = read_buffer_length;

				// lock again
				dbgf("unit lock (msg) %d.", unit_state->id);
				pthread_mutex_lock(&state->comm_mutex);

				// send message if needed
				if(s) {
					for(j = 0; j < MAX_PENDING_MESSAGES; j++) {
						if(state->net.send_pending[j].socket_id == -1)
							break;
					}
					if(j == MAX_PENDING_MESSAGES) {
						dbg("overwrite first message due to full send queue");
						j = 0;
					}
					dbgf("add send task id %d by %d to %d: %.*s", j, unit_state->id, socket_id,
							send_buffer_length, send_buffer);
					// add seperator at the end
					for(s = 0; s < 3; s++) {
						send_buffer[send_buffer_length + s] = '+';
					}
					send_buffer_length += 3;
					state->net.task[socket_id] = 2;
					state->net.send_pending[j].socket_id = socket_id;
					state->net.send_pending[j].buffer_length = send_buffer_length;
					dbgf("len=%d+3", send_buffer_length);
					memcpy(state->net.send_pending[j].buffer, send_buffer, send_buffer_length);
					dbgf("queue send task %d by %d: %.*s (%d)", j, socket_id,
							state->net.send_pending[j].buffer_length, state->net.send_pending[j].buffer,
							state->net.send_pending[j].buffer_length);
					// TODO may need to set send_buffer_length to 0
					s = 0;
				}
				break;
			}
		}

		dbgf("unit signal m %d.", unit_state->id);
		pthread_cond_signal(&state->comm_cond);

		dbgf("unit wait %d.", unit_state->id);
		pthread_cond_wait(&state->pool_cond, &state->comm_mutex);
	} while(1);
	dbgf("unit unlock %d.", unit_state->id);
	pthread_mutex_unlock(&state->comm_mutex);

exit:
	dbgf("Stopping unit thread %d.", unit_state->id);
	pthread_exit(NULL);
}
