#ifndef USAGE_H
#define USAGE_H

/* returns 0 if parameters are correct */
int print_usage(int argc, char **argv);
/* get client bind address parameter or default one */
long getClientBindAddress();
/* get client bind port parameter or default one */
int getClientBindPort();
/* get promoter bind address parameter or default one */
long getPromoterBindAddress();
/* get promoter bind port parameter or default one */
int getPromoterBindPort();

#endif
