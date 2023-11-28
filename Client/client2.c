#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "client2.h"
#include "awale.h"
#include "../request.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

enum State {MENU, CONNECTION_REQUEST, CHOOSE_PLAYER, CONNECTED, GAME, END};
typedef enum State State;

GameState gameState;

State state = MENU;

static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];

   fd_set rdfs;

   /* send our name */
   write_server(sock, name);

   menu_initial();
   //state = GAME;

   while(1)
   {
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the socket */
      FD_SET(sock, &rdfs);

      if(select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         fgets(buffer, BUF_SIZE - 1, stdin);
         {
            char *p = NULL;
            p = strstr(buffer, "\n");
            if(p != NULL)
            {
               *p = 0;
            }
            else
            {
               /* fclean */
               buffer[BUF_SIZE - 1] = 0;
            }
         }
         if(strlen(buffer) > 0){
            user_update(sock, buffer);
         }
         //write_server(sock, buffer);
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* server down */
         if(n == 0)
         {
            printf("Server disconnected !\n");
            break;
         }else{
            server_update(sock, buffer);
         }

         //puts(buffer);
      }
   }

   end_connection(sock);
}

static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   struct hostent *hostinfo;

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      fprintf (stderr, "Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *) hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(connect(sock,(SOCKADDR *) &sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_server(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buffer[n] = 0;

   return n;
}

static void write_server(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

void user_update(SOCKET sock,char* buffer)
{
   switch (state) {
      case MENU:
         /*
         1. List players
         */
         if(strcmp(buffer, "1") == 0){
            printf("List players\n");
            write_server(sock, actions[LIST_PLAYERS]);
            state = CHOOSE_PLAYER;
            printf("[STATE] CHOOSE_PLAYER\n");
         }
         break;
      case CONNECTION_REQUEST:
         if(strcmp(buffer, "y") == 0){
            char temp[BUF_SIZE] = "";
            strcat(&temp, "connectionResponse\n");
            strcat(&temp, "true\n");
            write_server(sock, &temp);
         }
         else if(strcmp(buffer, "n") == 0){
            state = MENU;
            printf("[STATE] MENU\n");
            char temp[BUF_SIZE] = "";
            strcat(&temp, "connectionResponse\n");
            strcat(&temp, "false\n");
            write_server(sock, &temp);
         }
         break;
      case CHOOSE_PLAYER:
         char temp[BUF_SIZE] = "";
         strcat(&temp, "connect\n");
         strcat(&temp, buffer);
         write_server(sock, &temp);
         break;
      case CONNECTED:
         /*
         0. Back to menu
         1. Choose player
         */
         menu_connected();
         if(strcmp(buffer, "1") == 0){
            write_server(sock, actions[START_GAME]);
         }
         break;
      case GAME:
         // when the user plays a move he enters the hole letter between A and F or a and f
         if(strlen(buffer) == 2 && (buffer[0] >= 'A' && buffer[0] <= 'F') || (buffer[0] >= 'a' && buffer[0] <= 'f')){
            char temp[BUF_SIZE];
            strcat(&temp, actions[MOVE]);
            strcat(&temp, buffer);
            printf("%s\n", temp);
            write_server(sock, buffer);
         }
         break;
      default:
         break;
   }
}

void server_update(SOCKET sock, char* buffer)
{
   // copy buffer
   char temp[BUF_SIZE];
   strcpy(temp, buffer);
   char* cmd = strtok(temp, "\n");
   printf("[SERVER COMMAND] %s\n", cmd);
   switch (state) {
      case MENU:
         if(strcmp(cmd, "connect") == 0){
            state = CONNECTION_REQUEST;
            printf("[STATE] CONNECTION_REQUEST\n");
            char* token = strtok(buffer, "\n");
            token = strtok(NULL, "\n");
            printf("Connection request from %s\n", token);
            printf("Accept ? (y/n)\n");
         }
         break;
      case CONNECTION_REQUEST:
         if(strcmp(cmd, "connected") == 0){
            char* token = strtok(buffer, "\n");
            token = strtok(NULL, "\n");
            if(strcmp(token, "true") == 0){
               state = CONNECTED;
               printf("Connected\n");
            }
            else{
               printf("Connection with player failed\n");
               state = MENU;
            }
         }
         break;
      case CHOOSE_PLAYER:
         if(strcmp(cmd, "listPlayers") == 0){
            displayPlayers(buffer);
            printf("Choose player : \n");
         }
         else if(strcmp(cmd, "connected") == 0)
         {
            char* token = strtok(buffer, "\n");
            token = strtok(NULL, "\n");
            if(strcmp(token, "true") == 0){
               state = CONNECTED;
               printf("Connected\n");
            }
            else{
               printf("Connection with player failed\n");
               state = MENU;
            }
         }
         break;
      case CONNECTED:
         if(strcmp(cmd, "startGame") == 0){
            state = GAME;
            printf("Game started\n");
         }
         break;
      case GAME:
         if(strcmp(cmd, "gameState") == 0){
            // refresg
            GameState gameState;
            deserializeGameState(buffer, &gameState);
            display(&gameState);
         }

         break;
      default:
         break;
   }
}

void menu_initial()
{
      printf("Bienvenue sur le jeu Awale!\n\n");

      printf("Menu :\n");
      printf("1. Se connecter à un autre joueur\n");
}

void menu_connected()
{
   printf("Menu :\n");
   printf("Entrez le numéro du joueur : \n");
}

static void action(SOCKET sock, char* buffer){
   // char *action = strtok(buffer, " ");
   // char newBuffer[BUF_SIZE];
   //  strcpy(newBuffer, buffer + strlen(action) + 1);
    if (strcmp(buffer, "listPlayers"))
    {
      printf("hello listPlayers\n");
      read_server(sock, buffer);
      
      while (!strcmp(buffer, "end")) {
         printf ( "Player : %s\n", buffer);
         read_server(sock, buffer);
      }
      printf ( "%s\n", buffer );
    }
}

static void displayPlayers(char* buffer){
   printf("Players online :\n");
   char* players = strtok(buffer, "\n");
   players = strtok ( NULL, "\n" );
   while ( players != NULL ) {
      printf ( "%s\n", players );
      // On demande le token suivant.
      players = strtok ( NULL, "\n" );
   }
}

/**
 * @brief Deserialize a game state from a buffer
 * 
 * @param buffer 
 * @param gameState 
 * @return int - EXIT_SUCCESS if success, EXIT_FAILURE otherwise
 */
int deserializeGameState(const char* buffer, GameState* gameState) {
   const char * separators = "\n ";
   char* token = strtok(buffer, separators);
   if(strcmp(token, "gameState") != 0){
      printf("Error parsing game state\n");
      return EXIT_FAILURE;
   }

   int row = 0;
   int col = 0;
   while( token != NULL && col < NUM_HOLES && row < 2) {
      token = strtok ( NULL, separators );
      gameState->board[row][col] = atoi(token);
      col++;
      if(col == NUM_HOLES){
         col = 0;
         row++;
      }
   }
   token = strtok ( NULL, separators );
   gameState->currentPlayer = atoi(token);
   
   token = strtok ( NULL, separators );
   gameState->score[0] = atoi(token);

   token = strtok ( NULL, separators );
   gameState->score[1] = atoi(token);

   return EXIT_SUCCESS;  
}

/**
 * @brief  Serialize a game state into a buffer
 * 
 * @param gameState 
 * @param buffer 
 * @return int - EXIT_SUCCESS if success, EXIT_FAILURE otherwise
 */
int serializeGameState(const GameState* gameState, char* buffer){
   char temp[BUF_SIZE];
   strcpy(buffer, "gameState\n");
   for(int i = 0; i < NUM_HOLES; i++){
      sprintf(temp, "%d ", gameState->board[0][i]);
      strcat(buffer, temp);
   }
   strcat(buffer, "\n");
   for(int i = 0; i < NUM_HOLES; i++){
      sprintf(temp, "%d ", gameState->board[1][i]);
      strcat(buffer, temp);
   }
   strcat(buffer, "\n");
   sprintf(temp, "%d\n", gameState->currentPlayer);
   strcat(buffer, temp);
   sprintf(temp, "%d\n", gameState->score[0]);
   strcat(buffer, temp);
   sprintf(temp, "%d\n", gameState->score[1]);
   strcat(buffer, temp);
   return EXIT_SUCCESS;
}



int main(int argc, char **argv)
{
   if(argc < 2)
   {
      printf("Usage : %s [address] [pseudo]\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();

   app(argv[1], argv[2]);

   end();

   return EXIT_SUCCESS;
}
