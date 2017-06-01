#ifndef CONSTANTS_H
#define CONSTANTS_H

/* must be IPv4 long representation, see inet_addr(3) and in_addr
   16777343 for 127.0.0.1; 0 for 0.0.0.0 */
#define DEFAULT_BIND_ADDRESS 16777343
/* for displaying in usage */
#define DEFAULT_BIND_ADDRESS_STRING "127.0.0.1"
/* must be 1024-9999 */
#define DEFAULT_CLIENT_BIND_PORT 5180
#define DEFAULT_PROMOTER_BIND_PORT 5181
/* number of thread pool unis */
#define THREAD_POOL_UNITS 4

/* internal constants */

/* id: 0-99 */
#define MAX_CLIENTS 100
#define SOCKET_BUFFER_MAX_LENGTH 1024

#define N_STATE_TASK_NONE 0
#define N_STATE_TASK_READ 1

#endif
