#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */

#include "../awale/awale.h"

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

#define BLACK "\033[1;30m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m"
#define RESET "\033[0m"

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
static void menu_choose_game();
static void menu_watch_game();
static void menu_connection();
static void menu_connected();
static void menu_game();
static void menu_look_bio();
static void menu_edit_bio();
static void displayPlayers(int bio, char* buffer);
static void displayGames(char* buffer);
static int deserializeGameState(char* buffer, GameState* gameState);
static void clear();
int letterToHole(char letter);


#endif /* guard */
