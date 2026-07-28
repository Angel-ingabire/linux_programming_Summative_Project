# =============================================================================
# Makefile for TCP Client-Server Monitoring System
# =============================================================================
# Usage:
#   make all      - Build both server and client
#   make server   - Build server only
#   make client   - Build client only
#   make run-server - Build and run server
#   make run-client - Build and run client (connects to 127.0.0.1:8080)
#   make clean    - Remove compiled binaries
# =============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS_SERVER = -pthread

.PHONY: all server client run-server run-client clean

all: server client

server: server.c
	$(CC) $(CFLAGS) $(LDFLAGS_SERVER) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

run-server: server
	./server

run-client: client
	./client 127.0.0.1 8080

clean:
	rm -f server client

