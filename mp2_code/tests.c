#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

struct vec_table
{
    int distance;
    short int path[100];
};

struct vec_table forward_table[256];
int neighborsCosts[255];

int main()
{
    char costsFile[4 + sizeof(int)];
    char globalMyID[4] = "1";
    char *search = " ";
    strcpy(costsFile, "costs");
    memcpy(costsFile + 5, globalMyID, sizeof(int));
    FILE *readCost = fopen(costsFile, "r");
    char line[100];
    printf("Modified String: %s\n", costsFile);
    int i = 0;
    while (fgets(line, sizeof(line), readCost))
    {
        char *node = strtok(line, " ");
        char *cost = strtok(NULL, "\r");
        if (cost != NULL)
        {
            neighborsCosts[atoi(node)] = atoi(cost);
        }
        else
        {
            neighborsCosts[atoi(node)] = 1;
        }
        i++;
    }
    printf("%hi\n", neighborsCosts[5]);
    fclose(readCost);
    return 0;
}
