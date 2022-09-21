all: vec

vec: main.c monitor_neighbors.c logging.c
	gcc -pthread -o vec_router main.c monitor_neighbors.c logging.c

.PHONY: clean
clean:
	rm *.o vec_router
