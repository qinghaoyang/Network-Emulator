#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "logging.h"

struct vec_table
{
	int distance;
	short int path[256];
};

void *announceToNeighbors(void *unusedParam);
void *monitorConnections(void *unusedParam);
void listenForNeighbors();
