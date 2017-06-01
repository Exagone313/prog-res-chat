#include "state.h"

void state_init(m_state *state) {
	int i;

	state->quit = 0;
	state->net.connected = 0;
	for(i = 0; i < MAX_CLIENTS; i++) {
		state->client_socket_id[i] = -1;
		state->net.client[i] = 0;
		state->net.task[i] = 0;
		state->net.buffer_length[i] = 0;
	}
}
