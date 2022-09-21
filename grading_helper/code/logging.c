#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char log_buf[1000];
extern char *fileName;

void write_log(char *buf, int buf_length)
{
    FILE *fp = fopen(fileName, "a");
    fwrite(buf, sizeof(char), buf_length, fp);
    fclose(fp);
}

void log_sending(short int dest, short int nexthop, char *message)
{
    memset(log_buf, '\0', 1000);
    sprintf(log_buf, "sending packet dest %d nexthop %d message %s\n", dest, nexthop, message);
    write_log(log_buf, strlen(log_buf));
}

void log_forwarding(short int dest, short int nexthop, char *message)
{
    memset(log_buf, '\0', 1000);
    sprintf(log_buf, "forward packet dest %d nexthop %d message %s\n", dest, nexthop, message);
    write_log(log_buf, strlen(log_buf));
}

void log_receiving(char *message)
{
    memset(log_buf, '\0', 1000);
    sprintf(log_buf, "receive packet message %s\n", message);
    write_log(log_buf, strlen(log_buf));
}

void log_unreachable(short int dest)
{
    memset(log_buf, '\0', 1000);
    sprintf(log_buf, "unreachable dest %d\n", dest);
    write_log(log_buf, strlen(log_buf));
}

void log_nothing(const char *buf)
{
    memset(log_buf, '\0', 1000);
    write_log(log_buf, strlen(log_buf));
}

void log_vector(short int dest, int distance, int cost, int path)
{
    memset(log_buf, '\0', 1000);
    sprintf(log_buf, "dest %d distance %d cost %d path %d\n", dest, distance, cost, path);
    write_log(log_buf, strlen(log_buf));
}