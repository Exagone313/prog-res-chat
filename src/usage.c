#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usage.h"
#include "constants.h"

static unsigned long client_address = DEFAULT_BIND_ADDRESS;
static int client_port = DEFAULT_CLIENT_BIND_PORT;
static unsigned long promoter_address = DEFAULT_BIND_ADDRESS;
static int promoter_port = DEFAULT_PROMOTER_BIND_PORT;

int print_usage(const int argc, const char **argv)
{
	struct in_addr ia;

	if(argc == 1)
		return 0;

	if(argc <= 5) {
		if(argv[1][0] == '-')
			goto usage;
		if(argv[1][0] != '_') {
			if(inet_pton(AF_INET, argv[1], &ia) == 0)
				goto usage;
			client_address = ia.s_addr;
		}
		if(argc > 2 && argv[2][0] != '_') {
			client_port = atoi(argv[2]);
			if(client_port < 1024 || client_port > 9999)
				goto usage;
		}
		if(argc > 3 && argv[3][0] != '_') {
			if(inet_pton(AF_INET, argv[3], &ia) == 0)
				goto usage;
			promoter_address = ia.s_addr;
		}
		if(argc > 4 && argv[4][0] != '_') {
			promoter_port = atoi(argv[4]);
			if(promoter_port < 1024 || promoter_port > 9999)
				goto usage;
		}

		if((client_address == promoter_address
					|| client_address == 0
					|| promoter_address == 0)
				&& client_port == promoter_port)
			goto usage;
		return 0;
	}

usage:
	fprintf(stderr, "Usage: %s [client-address [client-port [promoter-address [promoter-port]]]]\n"
			"\n"
			"\t_:\t\t\tDefault value\n"
			"\tclient-address:\t\tBind address for clients\t(Default: %s)\n"
			"\tclient-port:\t\tBind port for clients\t\t(Default: %d)\n"
			"\tpromoter-address:\tBind address for promoters\t(Default: %s)\n"
			"\tpromoter-port:\t\tBind port for promoters\t\t(Default: %d)\n"
			"\n"
			"\tAddresses must be IPv4\n"
			"\tBoth ports should be different, and 1024 <= port <= 9999\n",
			argv[0], DEFAULT_BIND_ADDRESS_STRING, DEFAULT_CLIENT_BIND_PORT,
			DEFAULT_BIND_ADDRESS_STRING, DEFAULT_PROMOTER_BIND_PORT);
	return 1;
}

unsigned long getClientBindAddress()
{
	return client_address;
}

int getClientBindPort() {
	return client_port;
}

unsigned long getPromoterBindAddress()
{
	return promoter_address;
}

int getPromoterBindPort() {
	return promoter_port;
}
