#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include "logging.h"
#include "monitor_neighbors.h"

#define ANNOUNCE_INTERVAL 999000 //microseconds

extern int globalMyID;
//last time you heard from each node. TODO: you will want to monitor this
//in order to realize when a neighbor has gotten cut off from you.
extern struct timeval globalLastHeartbeat[256];

//our all-purpose UDP socket, to be bound to 10.1.1.globalMyID, port 7777
extern int globalSocketUDP;
//pre-filled for sending to 10.1.1.0 - 255, port 7777
extern struct sockaddr_in globalNodeAddrs[256];

extern struct vec_table forward_table[256];
extern short int neighborsCosts[256];

extern char *fileName;
extern char *costName;

//Yes, this is terrible. It's also terrible that, in Linux, a socket
//can't receive broadcast packets unless it's bound to INADDR_ANY,
//which we can't do in this assignment.

void hackyBroadcast(const char *buf, int length)
{
	int i;
	for (i = 0; i < 256; i++)
		if (i != globalMyID) //(although with a real broadcast you would also get the packet yourself)
			sendto(globalSocketUDP, buf, length, 0,
				   (struct sockaddr *)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}

void hackyBroadcast2(int from, const char *buf, int length)
{
	int i;
	for (i = 0; i < 256; i++)
		if (i != from && i != globalMyID)
			sendto(globalSocketUDP, buf, length, 0, (struct sockaddr *)&globalNodeAddrs[i], sizeof(globalNodeAddrs[i]));
}

void parseSend(char *buf, short int *destID, char *message, int buf_length, int msg_length)
{
	buf += 4;
	memcpy(destID, buf, sizeof(short int));
	*destID = ntohs(*destID);
	memcpy(message, buf + sizeof(short int), msg_length);
}

void parseVec(char *buf, short int *destID, int *distance, short int *path, int buf_length, int path_length)
{
	buf += 3;
	memcpy(destID, buf, sizeof(short int));
	memcpy(distance, buf + sizeof(short int), sizeof(int));
	memcpy(path, buf + sizeof(short int) + sizeof(int), path_length * sizeof(short int));
}

void *announceToNeighbors(void *unusedParam)
{
	log_nothing();
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 999000 * 1000; //300 ms
	// vec + destID + distance + path
	while (1)
	{
		char vec_buf[3 + sizeof(short int) + sizeof(int) + sizeof(short int)];
		short int no_destID = htons(globalMyID);
		int no_distance = htonl(forward_table[globalMyID].distance);
		short int no_path = htons(forward_table[globalMyID].path[0]);
		strcpy(vec_buf, "vec");
		memcpy(vec_buf + 3, &no_destID, sizeof(short int));
		memcpy(vec_buf + 3 + sizeof(short int), &no_distance, sizeof(int));
		memcpy(vec_buf + 3 + sizeof(short int) + sizeof(int), &no_path, sizeof(short int));
		hackyBroadcast(vec_buf, 3 + sizeof(short int) + sizeof(int) + sizeof(short int));
		nanosleep(&sleepFor, 0);
		// int i = 0;
		// for (i = 0; i < 256; i++)
		// {
		// 	no_destID = htons(i);
		// 	no_distance = htonl(forward_table[i].distance);
		// 	if (no_distance > -1)
		// 	{
		// 		int end = 0;
		// 		short int no_path[256];
		// 		while (forward_table[i].path[end] > -1)
		// 		{
		// 			no_path[end] = htons(forward_table[i].path[end]);
		// 			end++;
		// 		}
		// 		char vec_buf[3 + sizeof(short int) + sizeof(int) + end * sizeof(short int)];
		// 		strcpy(vec_buf, "vec");
		// 		memcpy(vec_buf + 3, &no_destID, sizeof(short int));
		// 		memcpy(vec_buf + 3 + sizeof(short int), &no_distance, sizeof(int));
		// 		memcpy(vec_buf + 3 + sizeof(short int) + sizeof(int), &no_path, end * sizeof(short int));
		// 		hackyBroadcast(vec_buf, 3 + sizeof(short int) + sizeof(int) + end * sizeof(short int));
		// 	}
		// }
	}
}

void *monitorConnections(void *unusedParam)
{
	int i;
	struct timeval current;
	struct timespec sleepFor;
	sleepFor.tv_sec = 0;
	sleepFor.tv_nsec = 500 * 1000 * 1000; //500 ms

	while (1)
	{
		for (i = 0; i < 256; i++)
		{
			if (i != globalMyID)
			{
				gettimeofday(&current, 0);
				if ((current.tv_sec - globalLastHeartbeat[i].tv_sec) * 1000000 + current.tv_usec - globalLastHeartbeat[i].tv_usec > 1.5 * ANNOUNCE_INTERVAL &&
					forward_table[i].path[1] == i)
				{
					forward_table[i].distance = -1;
					forward_table[i].path[0] = -1;
					forward_table[i].path[1] = -1;
					int buf_length = 3 + sizeof(short int) + sizeof(int) + sizeof(short int);
					char vec_buf[buf_length];
					short int no_destID = htons(i);
					int no_distance = htonl(-1);
					short int no_path = htons(-1);
					strcpy(vec_buf, "vec");
					memcpy(vec_buf + 3, &no_destID, sizeof(short int));
					memcpy(vec_buf + 3 + sizeof(short int), &no_distance, sizeof(int));
					memcpy(vec_buf + 3 + sizeof(short int) + sizeof(int), &no_path, sizeof(short int));
					hackyBroadcast2(i, vec_buf, buf_length);
					// log_unreachable(i);
					for (int j=0; j < 256; j++)
					{
						if (j != i && j != globalMyID)
						{
							if (forward_table[j].path[1] == i)
							{
								forward_table[j].distance = -1;
								no_destID = htons(j);
								strcpy(vec_buf, "vec");
								memcpy(vec_buf + 3, &no_destID, sizeof(short int));
								memcpy(vec_buf + 3 + sizeof(short int), &no_distance, sizeof(int));
								memcpy(vec_buf + 3 + sizeof(short int) + sizeof(int), &no_path, sizeof(short int));
								hackyBroadcast2(i, vec_buf, buf_length);
								// log_unreachable(j);
							}
						}
					}
				}

			}
		}
		// for (i = 0; i < 256; i++)
		// {
		// 	if (i != globalMyID)
		// 	{
		// 		int previous = forward_table[i].path[1];
		// 		if (previous >= 0 && forward_table[previous].distance == -1)
		// 		{
		// 			forward_table[i].distance = -1;
		// 			log_unreachable(i);
		// 		}
		// 	}
		// }
		nanosleep(&sleepFor, 0);
	}
}

void listenForNeighbors()
{
	char fromAddr[100];
	struct sockaddr_in theirAddr;
	socklen_t theirAddrLen;
	unsigned char recvBuf[1000];

	int bytesRecvd;
	while (1)
	{
		theirAddrLen = sizeof(theirAddr);
		if ((bytesRecvd = recvfrom(globalSocketUDP, recvBuf, 1000, 0,
								   (struct sockaddr *)&theirAddr, &theirAddrLen)) == -1)
		{
			perror("connectivity listener: recvfrom failed");
			exit(1);
		}
		// log_recvBuf(bytesRecvd);
		inet_ntop(AF_INET, &theirAddr.sin_addr, fromAddr, 100);

		int heardFrom = -1;
		if (strstr(fromAddr, "10.1.1."))
		{
			heardFrom = atoi(
				strchr(strchr(strchr(fromAddr, '.') + 1, '.') + 1, '.') + 1);

			//TODO: this node can consider heardFrom to be directly connected to it; do any such logic now.
			neighborsCosts[heardFrom] = 1;
			FILE *readCost = fopen(costName, "r");
			char line[100];
			while (fgets(line, sizeof(line), readCost))
			{
				char *node = strtok(line, " ");
				char *cost = strtok(NULL, "\r");
				neighborsCosts[atoi(node)] = atoi(cost);
			}
			fclose(readCost);
			// }
			//record that we heard from heardFrom just now.
			gettimeofday(&globalLastHeartbeat[heardFrom], 0);
		}

		//Is it a packet from the manager? (see mp2 specification for more details)
		//send format: 'send'<4 ASCII bytes>, destID<net order 2 byte signed>, <some ASCII message>
		if (!strncmp(recvBuf, "send", 4))
		{
			//TODO send the requested message to the requested destination node
			// parse received buffer
			short int destID;
			int msg_length = bytesRecvd - 4 - sizeof(short int);
			char message[msg_length + 1];
			parseSend(recvBuf, &destID, message, bytesRecvd, msg_length);
			message[msg_length] = '\0';
			if (globalMyID == destID)
			{
				// receive packet message
				printf("received message");
				log_receiving(message);
			}
			else
			{
				short int nexthop = forward_table[destID].path[1];
				printf("%d", nexthop);

				if (forward_table[destID].distance == -1)
				{
					// unreachable
					log_unreachable(destID);
				}
				else
				{
					sendto(globalSocketUDP, recvBuf, bytesRecvd, 0, (struct sockaddr *)&globalNodeAddrs[nexthop], sizeof(globalNodeAddrs[nexthop]));
					if (strncmp(fromAddr, "10.0.0.10", 9) == 0)
					{
						log_sending(destID, nexthop, message);
					}
					else
					{
						log_forwarding(destID, nexthop, message);
					}
				}
			}
		}
		//'cost'<4 ASCII bytes>, destID<net order 2 byte signed> newCost<net order 4 byte signed>
		else if (!strncmp(recvBuf, "cost", 4))
		{
			//TODO record the cost change (remember, the link might currently be down! in that case,
			//this is the new cost you should treat it as having once it comes back up.)
			// ...
		}

		//TODO now check for the various types of packets you use in your own protocol
		//else if(!strncmp(recvBuf, "your other message types", ))
		// ...

		else if (!strncmp(recvBuf, "vec", 3))
		{
			short int no_myID = htons(globalMyID);
			short int no_destID;
			int no_distance;
			int path_length = (bytesRecvd - 3 - sizeof(short int) - sizeof(int)) / sizeof(short int);
			short int no_path[path_length];
			parseVec(recvBuf, &no_destID, &no_distance, no_path, bytesRecvd, path_length);
			short int path[path_length];
			int i;
			int cycle = 0;
			for (i = 0; i < path_length; i++)
			{
				path[i] = ntohs(no_path[i]);
				if (path[i] == globalMyID)
				{
					cycle = 1;
					break;
				}
			}
			if (cycle == 1)
				continue;
			short int destID = ntohs(no_destID);
			int distance = ntohl(no_distance);
			if (distance == -1)
			{
				if (forward_table[destID].path[1] == heardFrom)
				{
					forward_table[destID].distance = -1;
					// log_vector(destID, distance, heardFrom, path, path_length);
					hackyBroadcast2(heardFrom, recvBuf, bytesRecvd);
				}
			}
			else
			{
				int total_distance = distance + neighborsCosts[heardFrom];
				int no_total_distance = htonl(total_distance);
				if (total_distance < forward_table[destID].distance ||
					forward_table[destID].distance == -1 ||
					(total_distance == forward_table[destID].distance &&
					ntohs(no_path[0]) < forward_table[destID].path[1]) ||
					ntohs(no_path[0]) == forward_table[destID].path[1])
				{
					forward_table[destID].distance = total_distance;
					forward_table[destID].path[0] = globalMyID;
					// log_vector(destID, distance, heardFrom, path, path_length);
					int i;
					for (i = 0; i < path_length; i++)
					{
						forward_table[destID].path[i + 1] = ntohs(no_path[i]);
						// log_vector(destID, distance, neighborsCosts[heardFrom], forward_table[destID].path[i + 1]);
					}

					for (i = path_length + 1; i < 256; i++)
						forward_table[destID].path[i] = -1;
					int buf_length = 3 + sizeof(short int) + sizeof(int) + (path_length + 1) * sizeof(short int);
					char vec_buf[buf_length];
					strcpy(vec_buf, "vec");
					memcpy(vec_buf + 3, &no_destID, sizeof(short int));
					memcpy(vec_buf + 3 + sizeof(short int), &no_total_distance, sizeof(int));
					memcpy(vec_buf + 3 + sizeof(short int) + sizeof(int), &no_myID, sizeof(short int));
					memcpy(vec_buf + 3 + sizeof(short int) + sizeof(int) + sizeof(short int), &no_path, path_length * sizeof(short int));
					hackyBroadcast2(heardFrom, vec_buf, buf_length);
				}
			}
		}
	}
	//(should never reach here)
	close(globalSocketUDP);
}
