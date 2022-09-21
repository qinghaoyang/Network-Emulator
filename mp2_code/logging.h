#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void log_sending(short int dest, short int nexthop, char *message);

void log_forwarding(short int dest, short int nexthop, char *message);

void log_receiving(char *message);

void log_unreachable(short int dest);

void log_nothing();

void log_vector(short int dest, int distance, int cost, short int path[], int size);