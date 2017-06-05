#include "state.h"

void state_init(m_state *state) {
	int i, j;

	state->quit = 0;
	state->net.connected = 0;
	for(i = 0; i < MAX_CLIENTS; i++) {
		state->user_socket_id[i] = -1;
		state->net.client[i] = 0;
		state->net.task[i] = 0;
		state->net.buffer_length[i] = 0;
		for(j = 0; j < MAX_PENDING_NOTIFICATIONS; j++) {
			state->user_notification[i][j] = NULL;
		}
		state->user_id[i][0] = '\0';
		for(j = 0; j < MAX_CLIENTS; j++) {
			state->friend_request[i][j] = -1;
			state->user_friend[i][j] = 0;
		}
		state->user_sending_message[i] = NULL;
		for(j = 0; j < MAX_UNREAD_MESSAGES ; j++) {
			state->user_waiting_message[i][j] = NULL;
		}
	}
	for(i = 0; i < THREAD_POOL_UNITS; i++) {
		state->unit[i].master = state;
		state->unit[i].id = i;
	}
	for(i = 0; i < MAX_PENDING_MESSAGES; i++) {
		state->net.send_pending[i].socket_id = -1;
		state->net.recv_pending[i].socket_id = -1;
	}
	for(i = 0; i < MAX_NOTIFICATION_BUFFERS; i++) {
		state->user_notification_buffer[i].pointing = 0;
	}
	for(i = 0; i < MAX_CLIENTS * MAX_UNREAD_MESSAGES; i++) {
		state->user_message[i].user_id = -1;
	}
	for(i = 0; i < MAX_PROMOTERS; i++) {
		state->net.promoter[i] = 0;
	}
	for(i = 0; i < MAX_PENDING_ADS; i++) {
		state->ad_pending[i].buffer_length = 0;
	}
}
