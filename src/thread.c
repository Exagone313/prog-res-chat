#define _GNU_SOURCE

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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
						dbgf("cleared task %d, close=%d", j, state->net.send_pending[j].close);
						state->net.send_pending[j].socket_id = -1;
						if(state->net.send_pending[j].close)
							state->net.task[i] = 3;
					}
				}
			}
			if(state->net.task[i] == 2) {
				if(!has_more) {
					dbgf("sent all pending messages for client %d", i);
					state->net.task[i] = 1;
				} else
					dbgf("client %d still has more messages, delayed in writefds", i);
			}
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
			state->net.buffer_length[i] = 0; // empty buffer

			// if the client was logged in, manually disconnect the user
			for(j = 0; j < MAX_CLIENTS; j++) {
				if(state->user_socket_id[j] == i) {
					pthread_mutex_lock(&state->pool_mutex);

					state->user_socket_id[j] = -1;

					pthread_mutex_unlock(&state->pool_mutex);
					break;
				}
			}

			// remove pending messages (both send and recv) for that user
			for(j = 0; j < MAX_PENDING_MESSAGES; j++) {
				if(state->net.send_pending[j].socket_id == i) {
					state->net.send_pending[j].socket_id = -1;
					state->net.send_pending[j].buffer_length = 0;
				}
				if(state->net.recv_pending[j].socket_id == i) {
					state->net.recv_pending[j].socket_id = -1;
					state->net.recv_pending[j].buffer_length = 0;
				}
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

static void select_result(m_state *state, fd_set *readfds)
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
		// read message
		if(state->net.task[i] == 1 && FD_ISSET(state->net.client[i], readfds)) {
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
						if(r >= MESSAGE_BUFFER_MAX_LENGTH) { // disconnect client if message is too large
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
				if(state->net.buffer_length[i] >= SOCKET_BUFFER_MAX_LENGTH) // didn't get a message with all these chars, close the socket
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

#ifdef DEBUG
	// avoid to have to wait TIME_WAIT state in development
	setsockopt(state->net.client_server, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
#endif

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
		select_result(state, &readfds);
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

static void write_message(u_state *unit_state, int socket_id, char *send_buffer, int *send_buffer_length, int close)
{
	int i, j;
	m_state *state;

	state = unit_state->master;

	dbgf("lock write message %d", unit_state->id);
	pthread_mutex_lock(&state->comm_mutex);

	for(i = 0; i < MAX_PENDING_MESSAGES; i++) {
		if(state->net.send_pending[i].socket_id == -1)
			break;
	}
	if(i == MAX_PENDING_MESSAGES) {
		dbg("overwrite first message due to full send queue");
		i = 0;
	}

	dbgf("add send task id %d by %d to %d: %.*s", i, unit_state->id, socket_id,
			*send_buffer_length, send_buffer);

	// add separator at the end
	for(j = 0; j < 3; j++) {
		send_buffer[*send_buffer_length + j] = '+';
	}
	*send_buffer_length += 3;

	state->net.task[socket_id] = 2;
	state->net.send_pending[i].socket_id = socket_id;
	state->net.send_pending[i].buffer_length = *send_buffer_length;
	state->net.send_pending[i].close = close;
	dbgf("len=%d+3", *send_buffer_length);
	memcpy(state->net.send_pending[i].buffer, send_buffer, *send_buffer_length);

	dbgf("queue send task %d by %d: %.*s (%d)", i, socket_id,
			state->net.send_pending[i].buffer_length, state->net.send_pending[i].buffer,
			state->net.send_pending[i].buffer_length);

	dbgf("unlock write message %d", unit_state->id);
	pthread_mutex_unlock(&state->comm_mutex);
}

static int malformed_id(char *user_id) // the 8 characters must be set
{
	int i;
	char ch;

	for(i = 0; i < USER_ID_LENGTH; i++) {
		ch = user_id[i];
		if(ch < 48 || (ch > 57 && ch < 65) || (ch > 90 && ch < 97) || ch > 122)
			return 1;
	}
	return 0;
}

static int read_port(char *port) // the 4 characters must be set
{
	char copy[5] = {0};
	int res;

	memcpy(copy, port, 4);
	res = atoi(copy);
	if(res < 1024 || res > 9999)
		return 0;
	return res;
}

static int get_user_pos(char *user_id, char list[MAX_CLIENTS][USER_ID_LENGTH]) // pool mutex must be locked
{
	int i;

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(list[i][0] == '\0' || memcmp(list[i], user_id, USER_ID_LENGTH) == 0)
			return i;
	}

	return -1;
}

static int get_user_number(char list[MAX_CLIENTS][USER_ID_LENGTH]) // pool mutex must be locked
{
	int i;

	for(i = 0; i < MAX_CLIENTS; i++) {
		if(list[i][0] == '\0')
			return i;
	}

	return MAX_CLIENTS;
}

static int user_password_to_int(char *user_password) // the 2 characters must be set
{
	return ((unsigned) user_password[0] << 8) + (unsigned) user_password[1];
}

static void read_message(u_state *unit_state, int socket_id, char *read_buffer, int read_buffer_length)
{
	int i, j, send_buffer_length, message_type, port, user_pos, user_number, user_id;
	char send_buffer[MESSAGE_BUFFER_MAX_LENGTH + 3] = {0};
	m_state *state;

	if(read_buffer_length < 5) // ignore messages too small
		return;

	state = unit_state->master;

	message_type = message_type_to_int(read_buffer);

	dbgf("lock pool %d.", unit_state->id);
	pthread_mutex_lock(&state->pool_mutex);

	// check if logged in
	user_id = -1;
	for(i = 0; i < MAX_CLIENTS; i++) {
		if(state->user_socket_id[i] == socket_id) {
			user_id = i;
			break;
		}
	}

	// check IQUIT
	if(message_type == IQUIT) {
		if(user_id != -1) { // logged in
			dbgf("disconnect user pos %d", i);
			state->user_socket_id[i] = -1;
		}

		dbgf("unlock pool %d.", unit_state->id);
		pthread_mutex_unlock(&state->pool_mutex);

		int_to_message_type(GOBYE, send_buffer);
		send_buffer_length = 5;
		write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 1);
		return;
	}

	dbgf("unlock pool %d.", unit_state->id);
	pthread_mutex_unlock(&state->pool_mutex);

	if(user_id == -1) // not logged in
		switch(message_type) {
			case REGIS: // note: connected directly after successful registration
				do {
					/*
					 * check length
					 * check spaces
					 * check that id is alphanumeric
					 * check that port is valid
					 * check that id is not already registered
					 * check that port is not taken, convert password to int
					 * login
					 */
					if(read_buffer_length != 22) // 5+1+8+1+4+1+2
						break;
					if(read_buffer[5] != ' ' || read_buffer[14] != ' ' || read_buffer[19] != ' ')
						break;
					if(malformed_id(read_buffer + 6))
						break;
					port = read_port(read_buffer + 15);
					if(port == 0)
						break;

					dbgf("received REGIS id=%.*s port=%d by %d", 8, read_buffer + 6, port, unit_state->id);

					dbgf("lock pool %d.", unit_state->id);
					pthread_mutex_lock(&state->pool_mutex);

					user_pos = get_user_pos(read_buffer + 6, state->user_id);
					if(user_pos == -1) {
						dbg("all user slots are taken");
						break;
					}
					if(state->user_id[user_pos][0] != '\0') {
						dbgf("user id=%.*s already registered", 8, read_buffer + 6);
						break;
					}

					// register
					memcpy(state->user_id[user_pos], read_buffer + 6, 8);
					state->user_password[user_pos] = user_password_to_int(read_buffer + 20);
					state->user_port[user_pos] = port;
					// FIXME vulnerability if the client disconnects before that message is read, until he reconnects
					state->user_addr[user_pos] = state->net.client_addr[socket_id];

					// login
					// FIXME vulnerability
					state->user_socket_id[user_pos] = socket_id;
					dbgf("logged on user pos %d", user_pos);

					dbgf("unlock pool %d.", unit_state->id);
					pthread_mutex_unlock(&state->pool_mutex);

					int_to_message_type(WELCO, send_buffer);
					send_buffer_length = 5;
					write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
					return;
				} while(0);
				int_to_message_type(GOBYE, send_buffer);
				send_buffer_length = 5;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 1);
				return;
			case CONNE:
				do {
					/*
					 * check length
					 * check spaces
					 * check that id is alphanumeric and is registered
					 * convert password to int and check that password matches
					 * login
					 */
					if(read_buffer_length != 17) // 5+1+8+1+2
						break;
					if(read_buffer[5] != ' ' || read_buffer[14] != ' ')
						break;
					if(malformed_id(read_buffer + 6))
						break;

					dbgf("lock pool %d.", unit_state->id);
					pthread_mutex_lock(&state->pool_mutex);

					user_pos = get_user_pos(read_buffer + 6, state->user_id);
					if(user_pos == -1 || state->user_id[user_pos][0] == '\0') {
						dbg("user id not found");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					if(user_password_to_int(read_buffer + 15) != state->user_password[user_pos]) {
						dbg("incorrect password");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					// login
					// FIXME vulnerability same as in REGIS
					state->user_socket_id[user_pos] = socket_id;
					dbgf("logged on user pos %d", user_pos);

					dbgf("unlock pool %d.", unit_state->id);
					pthread_mutex_unlock(&state->pool_mutex);

					int_to_message_type(HELLO, send_buffer);
					send_buffer_length = 5;
					write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
					return;
				} while(0);
				int_to_message_type(GOBYE, send_buffer);
				send_buffer_length = 5;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 1);
				return;
			// default: ignore unknown messages
		}
	else
		switch(message_type) {
			case LISTX: // user list
				/*
				 * RLIST NNN (char)
				 * LINUM 12345678
				 */
				dbgf("lock pool %d.", unit_state->id);
				pthread_mutex_lock(&state->pool_mutex);

				int_to_message_type(RLIST, send_buffer);
				user_number = get_user_number(state->user_id);
				send_buffer[5] = ' ';
				send_buffer[6] = '0' + (user_number / 100) % 10;
				send_buffer[7] = '0' + (user_number % 100) / 10;
				send_buffer[8] = '0' + user_number % 10;

				send_buffer_length = 9;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);

				// FIXME potential problem if the user disconnects while receiving the list as it unlocks comm_mutex
				for(i = 0; i < user_number; i++) {
					int_to_message_type(LINUM, send_buffer);
					send_buffer[5] = ' ';
					memcpy(send_buffer + 6, state->user_id[i], 8);
					send_buffer_length = 14;
					write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
				}

				dbgf("unlock pool %d.", unit_state->id);
				pthread_mutex_unlock(&state->pool_mutex);
				return;
			case FRIEX:
				do {
					/*
					 * check length
					 * check space
					 * check that id is alphanumeric and is registered and not current id
					 * check that the user is not already a friend
					 * check that there is no pending request made by the current user to that user
					 */
					if(read_buffer_length != 14) // 5+1+8
						break;
					if(read_buffer[5] != ' ')
						break;
					if(malformed_id(read_buffer + 6))
						break;

					dbgf("lock pool %d.", unit_state->id);
					pthread_mutex_lock(&state->pool_mutex);

					user_pos = get_user_pos(read_buffer + 6, state->user_id);
					if(user_pos == -1 || state->user_id[user_pos][0] == '\0' || user_pos == user_id) {
						dbg("user id not found or incorrect");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					// check not friend
					if(state->user_friend[user_pos][user_id]) {
						dbg("already friends");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					// check not already pending
					for(i = 0; i < MAX_CLIENTS; i++) {
						if(state->friend_request[user_pos][i] == -1
								|| state->friend_request[user_pos][i] == user_id)
							break;
					}
					if(i == MAX_CLIENTS || state->friend_request[user_pos][i] == user_id) {
						dbg("too many friend requests (how is that possible) or already pending request");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					// get next notification buffer available id
					for(i = 0; i < MAX_NOTIFICATION_BUFFERS; i++) {
						if(state->user_notification_buffer[i].pointing == 0)
							break;
					}
					if(i == MAX_NOTIFICATION_BUFFERS) {
						dbg("too many global notifications");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					// get next user notification available id
					for(j = 0; j < MAX_PENDING_NOTIFICATIONS; j++) {
						if(state->user_notification[user_pos][j] == NULL)
							break;
					}
					if(j == MAX_PENDING_NOTIFICATIONS) {
						dbg("too many user notifications");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					// save notification
					state->user_notification_buffer[i].pointing = 1;
					state->user_notification_buffer[i].buffer_length = 14;
					int_to_message_type(EIRFY, state->user_notification_buffer[i].buffer);
					state->user_notification_buffer[i].buffer[5] = ' ';
					memcpy(state->user_notification_buffer[i].buffer + 6, state->user_id[user_id], 8);

					// save notification pointer
					state->user_notification[user_pos][j] = state->user_notification_buffer
						+ i * sizeof(*state->user_notification_buffer);

					// add friend request
					state->friend_request[user_pos][i] = user_id;

					dbgf("unlock pool %d.", unit_state->id);
					pthread_mutex_unlock(&state->pool_mutex);

					int_to_message_type(FRIEY, send_buffer);
					send_buffer_length = 5;
					write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);

					// TODO udp notif
					return;
				} while(0);
				int_to_message_type(FRIEZ, send_buffer);
				send_buffer_length = 5;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
				return;
			case MESSX:
				do {
				} while(0);
				int_to_message_type(MESSZ, send_buffer);
				send_buffer_length = 5;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
				return;
			case FLOOX:
				do {
				} while(0);
				int_to_message_type(FLOOY, send_buffer);
				send_buffer_length = 5;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
				return;
			case CONSU:
				do {
					dbgf("lock pool %d.", unit_state->id);
					pthread_mutex_lock(&state->pool_mutex);

					if(state->user_notification[user_id][0] == NULL) {
						dbg("no notification");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					for(i = 1; i < MAX_PENDING_NOTIFICATIONS - 1; i++) {
						if(state->user_notification[user_id][0] == NULL)
							break;
					}

					(*state->user_notification[user_id][0]).pointing--;
					memcpy(send_buffer, (*state->user_notification[user_id][0]).buffer,
							(*state->user_notification[user_id][0]).buffer_length);
					memmove(state->user_notification[user_id],
							state->user_notification[user_id] + sizeof(*state->user_notification[user_id]),
							i);
					state->user_notification[user_id][i - 1] = NULL;

					dbgf("unlock pool %d.", unit_state->id);
					pthread_mutex_unlock(&state->pool_mutex);

					write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
					return;
				} while(0);
				int_to_message_type(NOCON, send_buffer);
				send_buffer_length = 5;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
				return;
			case OKIRF:
			case NOKRF:
				do {
					dbgf("lock pool %d.", unit_state->id);
					pthread_mutex_lock(&state->pool_mutex);

					if(state->friend_request[user_id][0] == -1) {
						dbg("no friend request");
						pthread_mutex_unlock(&state->pool_mutex);
						break;
					}

					// get next user notification available id
					for(j = 0; j < MAX_PENDING_NOTIFICATIONS; j++) {
						if(state->user_notification[state->friend_request[user_id][0]][j] == NULL)
							break;
					}
					if(j == MAX_PENDING_NOTIFICATIONS)
						dbg("too many user notifications");
					else {
						// save notification
						state->user_notification_buffer[i].pointing = 1;
						state->user_notification_buffer[i].buffer_length = 14;
						int_to_message_type((message_type == OKIRF) ? FRIEN : NOFRI,
								state->user_notification_buffer[i].buffer);
						state->user_notification_buffer[i].buffer[5] = ' ';
						memcpy(state->user_notification_buffer[i].buffer + 6,
								state->user_id[state->friend_request[user_id][0]], 8);

						// save notification pointer
						state->user_notification[state->friend_request[user_id][0]][j] =
							state->user_notification_buffer + i * sizeof(*state->user_notification_buffer);
					}

					if(message_type == OKIRF) { // make them friends
						state->user_friend[user_id][state->friend_request[user_id][0]] = 1;
						state->user_friend[state->friend_request[user_id][0]][user_id] = 1;
						// TODO udp notification
					}
					// TODO else udp notification

					// erase friend request
					memmove(state->friend_request[user_id], state->friend_request[user_id] + 1,
							MAX_CLIENTS - 1);
					state->friend_request[user_id][MAX_CLIENTS - 1] = -1;

					dbgf("unlock pool %d.", unit_state->id);
					pthread_mutex_unlock(&state->pool_mutex);
				} while(0);
				int_to_message_type(ACKRF, send_buffer);
				send_buffer_length = 5;
				write_message(unit_state, socket_id, send_buffer, &send_buffer_length, 0);
				return;
			// default: ignore unknown messages
		}
}

void *unit_thread_func(void *cls) // thread pool unit
{
	u_state *unit_state;
	m_state *state;
	int i, socket_id;
	char read_buffer[MESSAGE_BUFFER_MAX_LENGTH] = {0};
	int read_buffer_length;

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
				dbgf("read message from %d: %.*s (%d) by %d", state->net.recv_pending[i].socket_id,
						state->net.recv_pending[i].buffer_length, state->net.recv_pending[i].buffer,
						state->net.recv_pending[i].buffer_length, unit_state->id);
				// copy message
				read_buffer_length = state->net.recv_pending[i].buffer_length;
				memcpy(read_buffer, state->net.recv_pending[i].buffer, read_buffer_length);
				socket_id = state->net.recv_pending[i].socket_id;
				state->net.recv_pending[i].socket_id = -1;
				state->net.recv_pending[i].buffer_length = 0;
				// unlock
				dbgf("unit unlock (msg) %d.", unit_state->id);
				pthread_mutex_unlock(&state->comm_mutex);

				// read (and write) messages
				read_message(unit_state, socket_id, read_buffer, read_buffer_length);

				// lock again
				dbgf("unit lock (msg) %d.", unit_state->id);
				pthread_mutex_lock(&state->comm_mutex);

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
