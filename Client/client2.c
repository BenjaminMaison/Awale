#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "client2.h"
#include "../Serveur/awale.h"

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

enum State {MENU, CONNECTION, GAME, END, INVITATION};
typedef enum State State;

State state = MENU;

static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];

   fd_set rdfs;

   /* send our name */
   write_server(sock, name);

   menu_initial();

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
   char toSend[BUF_SIZE];
   switch (state) {
      case MENU:
            if(strcmp("1", buffer) == 0){
               write_server(sock, "getListPlayers:");
            }
            break;
      case CONNECTION:
            strcpy(toSend, "connectToPlayer:");
            strcat(toSend, buffer);
            write_server(sock, toSend);
            break;
      case INVITATION:
      if(strcmp("1", buffer) == 0){
         write_server(sock, "accept:");
      }else if(strcmp("2", buffer) == 0){
         write_server(sock, "refuse:");
         menu_initial();
      }
      break;
      case GAME:
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
   char* cmd = strtok(temp, ":");
   printf("Command : %s\n", cmd);
   switch (state) {
      case MENU:
         if(strcmp("listPlayers", cmd) == 0){
            displayPlayers(strtok(NULL, "\0"));
         }else if(strcmp("invite", cmd) == 0){
            menu_invitation(strtok(NULL, "\0"));
         }
         break;
      case CONNECTION:
         if(strcmp("error", cmd) == 0){
            printf("Veuillez entrer le numéro d'un joueur correct\n");
         }else if(strcmp("sent", cmd) == 0){
            state = INVITATION;
         }else if(strcmp("invite", cmd) == 0){
            menu_invitation();
         }
         break;
      case INVITATION:
         if(strcmp("accepted", cmd) == 0){
            printf("%s a accepté votre invitation !\n", strtok(NULL, "\0"));
         }else if(strcmp("refused", cmd) == 0){
            printf("%s a refusé votre invitation...\n", strtok(NULL, "\0"));
            menu_initial();
         }
         break;
      case GAME:
         if(strcmp("gameState", cmd) == 0){
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
      state = MENU;
      printf("Bienvenue sur le jeu Awale!\n\n");

      printf("Menu :\n");
      printf("1. Se connecter à un autre joueur\n");
}

void menu_invitation(char* name)
{
      state = INVITATION;
      printf("%s vous a envoyé une invitation\n", name);
      printf("1 - Accepter\n");
      printf("2 - Refuser\n");
}

void menu_connected()
{
   state = CONNECTION;
   printf("Menu :\n");
   printf("Entrez le numéro du joueur : \n");
}

static void displayPlayers(char* buffer){
   printf("Display players\n");
   char* token = strtok(buffer, ",");
   int i = 1;
   while(token != NULL){
      printf("Player %d: %s\n", i, token);
      i++;
      token = strtok(NULL, ",");
   }
   menu_connected();
}

static void connectToPlayer(SOCKET sock){

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
