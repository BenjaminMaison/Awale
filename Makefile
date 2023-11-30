CC = gcc
CFLAGS = -Wall

# List of source files
CLIENT_SRC = src/client/client2.c
SERVER_SRC = src/server/server2.c
AWALE_SRC = src/awale/awale.c
REQUEST_SRC = src/request.c
RESPONSE_SRC = src/response.c

# List of object files
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)
AWALE_OBJ = $(AWALE_SRC:.c=.o)

# Executables
CLIENT_EXEC = client
SERVER_EXEC = server

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_OBJ) $(AWALE_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_EXEC): $(SERVER_OBJ) $(AWALE_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_EXEC) $(SERVER_EXEC) $(CLIENT_OBJ) $(SERVER_OBJ) $(AWALE_OBJ)
