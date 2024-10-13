CC = gcc
CFLAGS = -Wall -g
CODE_DIR = code
CLIENT_DIR = client
SERVER_DIR = server
CLIENT_TARGET = $(CLIENT_DIR)/c
SERVER_TARGET = $(SERVER_DIR)/s
CLIENT_OBJS = $(CODE_DIR)/client.o $(CODE_DIR)/common.o
SERVER_OBJS = $(CODE_DIR)/server.o $(CODE_DIR)/common.o

all: $(CLIENT_TARGET) $(SERVER_TARGET) clean_objs

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS)

$(CODE_DIR)/client.o: $(CODE_DIR)/client.c $(CODE_DIR)/client.h $(CODE_DIR)/common.h
	$(CC) $(CFLAGS) -c $(CODE_DIR)/client.c -o $(CODE_DIR)/client.o

$(CODE_DIR)/server.o: $(CODE_DIR)/server.c $(CODE_DIR)/server.h $(CODE_DIR)/common.h
	$(CC) $(CFLAGS) -c $(CODE_DIR)/server.c -o $(CODE_DIR)/server.o

$(CODE_DIR)/common.o: $(CODE_DIR)/common.c $(CODE_DIR)/common.h
	$(CC) $(CFLAGS) -c $(CODE_DIR)/common.c -o $(CODE_DIR)/common.o

clean_objs:
	rm -f $(CODE_DIR)/client.o $(CODE_DIR)/server.o $(CODE_DIR)/common.o

clean:
	rm -f $(CODE_DIR)/client.o $(CODE_DIR)/server.o $(CODE_DIR)/common.o $(CLIENT_TARGET) $(SERVER_TARGET)