#ifndef STATE_H
#define STATE_H

#include <pthread.h>
#include "constants.h"

typedef struct m_state m_state; // master thread
typedef struct n_state n_state; // net thread
typedef struct u_state u_state; // thread pool unit
typedef struct message_task message_task;
typedef struct notification_buffer notification_buffer;

struct message_task {
	int socket_id; // position in m_state.net.client, -1 if task not set
	int close; // for send, 0 to send normally, 1 to send and close connection
	char buffer[MESSAGE_BUFFER_MAX_LENGTH + 3];
	int buffer_length;
};

struct notification_buffer {
	int pointing; // number of notifications pointing to this buffer, 0 for unset
	char buffer[MESSAGE_BUFFER_MAX_LENGTH];
	int buffer_length;
};

struct n_state {
	int connected; // number of connected clients
	int unlockpipe[2];
	int client_server; // listening socket for clients
	int promoter_server; // listening socket for promoters
	int client[MAX_CLIENTS]; // connected client's sockets
	unsigned long client_addr[MAX_CLIENTS]; // its address
	int task[MAX_CLIENTS]; // net tasks e.g. close socket
	char buffer[MAX_CLIENTS][SOCKET_BUFFER_MAX_LENGTH]; // received data, could be incomplete
	int buffer_length[MAX_CLIENTS]; // actual lengths
	message_task send_pending[MAX_PENDING_MESSAGES];
	message_task recv_pending[MAX_PENDING_MESSAGES];
};

struct u_state {
	m_state *master;
	int id;
};

struct m_state {
	pthread_t main; // main process to signal in case of failure
	pthread_attr_t thread_attr;
	int quit;
	n_state net;
	int user_socket_id[MAX_CLIENTS]; // for a user id, contains its position in net.client (-1 for disconnected)
	char user_id[MAX_CLIENTS][USER_ID_LENGTH];
	int user_password[MAX_CLIENTS];
	int user_port[MAX_CLIENTS]; // fixed at registration
	unsigned long user_addr[MAX_CLIENTS]; // saved on connection
	notification_buffer *user_notification[MAX_CLIENTS][MAX_PENDING_NOTIFICATIONS]; // NULL for unset
	notification_buffer user_notification_buffer[MAX_NOTIFICATION_BUFFERS];
	int friend_request[MAX_CLIENTS][MAX_CLIENTS]; // requests are ordered
	int user_friend[MAX_CLIENTS][MAX_CLIENTS]; // 1 if friend, 0 if not, both ways are set
	pthread_mutex_t comm_mutex;
	pthread_cond_t comm_cond;
	u_state unit[THREAD_POOL_UNITS];
	pthread_mutex_t pool_mutex;
	pthread_cond_t pool_cond;
};

void state_init(m_state *state);

#endif
