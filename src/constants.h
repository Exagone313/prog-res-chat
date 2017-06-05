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
/* to reserve memory instead of using memory allocation */
#define MAX_PENDING_MESSAGES 1000
#define MESSAGE_BUFFER_MAX_LENGTH 512
/* per user */
#define MAX_PENDING_NOTIFICATIONS 100
#define MAX_NOTIFICATION_BUFFERS MAX_CLIENTS * MAX_PENDING_NOTIFICATIONS
#define USER_MESSAGE_MAX_PARTS 5

/* protocol constants */

#define USER_ID_LENGTH 8
#define USER_MESSAGE_PART_LENGTH 200

/* macro utils */

#ifdef DEBUG
#define dbg(format) fprintf(stderr, "[DEBUG] " format "\n")
#define dbgf(format, ...) fprintf(stderr, "[DEBUG] " format "\n", ##__VA_ARGS__)
#else
#define dbg(format) while(0)
#define dbgf(format, ...) while(0)
#endif
#define err(format) fprintf(stderr, "[ERROR] " format "\n")
#define errf(format, ...) fprintf(stderr, "[ERROR] " format "\n", ##__VA_ARGS__)

#endif
