#ifndef STATE_H
#define STATE_H

#include <pthread.h>
#include "constants.h"

typedef struct m_state m_state; // master thread
typedef struct n_state n_state; // net thread
typedef struct u_state u_state; // thread pool unit

struct n_state {
	int connected; // number of connected clients
	int exitpipe;
	int client_server; // listening socket for clients
	int promoter_server; // listening socket for promoters
	int client[MAX_CLIENTS]; // connected client's sockets
	int task[MAX_CLIENTS]; // net tasks e.g. close socket
	char buffer[MAX_CLIENTS][SOCKET_BUFFER_MAX_LENGTH]; // received data, could be incomplete
	int buffer_length[MAX_CLIENTS]; // actual lengths
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
	char user_name[MAX_CLIENTS][8];
	pthread_mutex_t comm_mutex;
	pthread_cond_t comm_cond;
	u_state unit[THREAD_POOL_UNITS];
	pthread_mutex_t pool_mutex;
	pthread_cond_t pool_cond;
};

void state_init(m_state *state);

#endif
