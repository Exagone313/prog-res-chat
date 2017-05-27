#include <stdio.h>
#include "usage.h"

int main(const int argc, const char **argv)
{
	if(print_usage(argc, argv))
		return 1;

	printf("For clients: %ld:%d\n"
			"For promoters: %ld:%d\n",
			getClientBindAddress(), getClientBindPort(),
			getPromoterBindAddress(), getPromoterBindPort());
}
