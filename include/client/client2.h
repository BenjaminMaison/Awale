#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */

#include "../awale/awale.h"
#include "../request.h"

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#define CRLF     "\r\n"
#define PORT     1977

#define BUF_SIZE 1024

static void init(void);
static void end(void);
static void app(const char *address, const char *name);
static int init_connection(const char *address);
static void end_connection(int sock);
static int read_server(SOCKET sock, char *buffer);
static void write_server(SOCKET sock, const char *buffer);
static void user_update(SOCKET sock,char* buffer);
static void server_update(SOCKET sock,char* buffer);
static void menu_initial();
static void menu_invitation(char* name);
static void menu_connected();
static void displayPlayers(char* buffer);
static int deserializeGameState(const char* buffer, GameState* gameState);
static int serializeGameState(const GameState* gameState, char* buffer);


#endif /* guard */
