#ifndef STATE_H
#define STATE_H

#include <pthread.h>
#include "constants.h"

typedef struct m_state m_state; // master thread
typedef struct n_state n_state; // net thread
typedef struct u_state u_state; // thread pool unit

struct n_state { // TODO blocks on select
	int connected; // number of connected clients
	int client_server; // listening socket for clients
	int promoter_server; // listening socket for promoters
	int client[MAX_CLIENTS]; // connected client's sockets
	int task[MAX_CLIENTS]; // net tasks e.g. close socket
	char buffer[MAX_CLIENTS][SOCKET_BUFFER_MAX_LENGTH]; // received data, could be incomplete
	int buffer_length[MAX_CLIENTS]; // actual lengths
};

struct u_state { // TODO blocks on cond wait + shared mutex lock across units
	m_state *master;
};

struct m_state { // TODO blocks on cond wait + dedicated mutex lock, unlock as fast as possible
	pthread_t main; // main process to signal in case of failure
	pthread_attr_t thread_attr;
	int quit;
	n_state net;
	int client_socket_id[MAX_CLIENTS]; // for a user id, contains its position in net.client
	pthread_mutex_t comm_mutex;
	pthread_cond_t comm_cond;
	u_state unit[THREAD_POOL_UNITS];
	pthread_mutex_t pool_mutex;
	pthread_cond_t pool_cond;
};

void state_init(m_state *state);

#endif
