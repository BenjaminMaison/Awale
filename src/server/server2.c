#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../../include/server/server2.h"

 /* an array for all games */
int nbGames = 0;
Game games[MAX_CLIENTS/2];


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

static void app(void)
{
   SOCKET sock = init_connection();
   char  buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];

   fd_set rdfs;

   while(1)
   {
      int i = 0;  
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         printf("[NEW CONNECTION] %s connected to the server\n", c.name);
         clients[actual] = c;
         clients[actual].connectedTo = -1;
         clients[actual].gameID = -1;
         strncpy(clients[actual].bio, "Hello, lets play awale together :)", BIO_SIZE-1);

         actual++;
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);

               /* client disconnected */
               if(c == 0)
               {
                  if(client.connectedTo != -1){
                     char toSend[BUF_SIZE];
                     strncpy(toSend, "disconnected:", BUF_SIZE-1);
                     strncat(toSend, client.name, sizeof(toSend) - strlen(toSend) - 1);
                     clients[client.connectedTo].connectedTo = -1;
                     write_client(clients[client.connectedTo].sock, toSend);
                  }
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  // strncpy(buffer, client.name, BUF_SIZE - 1);
                  // strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                  // send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else
               {
                  action(buffer, clients, actual, i);
                  // send_message_to_all_clients(clients, client, actual, buffer, 0);
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   char message[BUF_SIZE];
   message[0] = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   printf("[CLIENT RESPONSE] %s\n", buffer);
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void action(const char *buffer, Client *clients, int actual, int clientID){
   char toSend[BUF_SIZE];
   SOCKET sock = clients[clientID].sock;
   char* token = strtok(buffer, ":");
   printf("[ACTION] %s\n", token);
   if(strcmp("getListPlayers", token) == 0){
      strncpy(toSend, "listPlayers:", BUF_SIZE-1);
      strncat(toSend, strtok(NULL, "\0"), sizeof(toSend) - strlen(toSend) - 1);
      strncat(toSend, ",", sizeof(toSend) - strlen(toSend) - 1);
      int i;
      for(i = 0; i<actual; i++){
         printf("%s\n",clients[i].name);
         strncat(toSend, clients[i].name, sizeof(toSend) - strlen(toSend) - 1);
         strncat(toSend, ",", sizeof(toSend) - strlen(toSend) - 1);
         if(i == clientID){
            strncat(toSend, "you", sizeof(toSend) - strlen(toSend) - 1);
         }else if(clients[i].connectedTo != -1){
            strncat(toSend, "already in a game", sizeof(toSend) - strlen(toSend) - 1);
         }else{
            strncat(toSend, "online", sizeof(toSend) - strlen(toSend) - 1);
         }
         strncat(toSend, ",", sizeof(toSend) - strlen(toSend) - 1);
      }
      write_client(sock, toSend);
      printf("%s\n",toSend);
   }else if(strcmp("connectToPlayer", token) == 0){
      token = strtok(NULL, "\0");
      int otherClientID = atoi(token)-1;
      if(clientID == otherClientID || otherClientID>=actual || otherClientID<0 || clients[otherClientID].connectedTo != -1){
         write_client(clients[clientID].sock, "error");
      }else{
         strncpy(toSend, "invite:", BUF_SIZE-1);
         strncat(toSend, clients[clientID].name, sizeof(toSend) - strlen(toSend) - 1);
         write_client(clients[otherClientID].sock, toSend);
         write_client(clients[clientID].sock, "sent:");
         clients[clientID].connectedTo = otherClientID;
         clients[otherClientID].connectedTo = clientID; 
      }
   }else if(strcmp("accept", token) == 0){
      int otherClientID = clients[clientID].connectedTo;
      strncpy(toSend, "accepted:", BUF_SIZE-1);
      strncat(toSend, clients[clientID].name, sizeof(toSend) - strlen(toSend) - 1);
      write_client(clients[otherClientID].sock, toSend);
   }else if(strcmp("refuse", token) == 0){
      int otherClientID = clients[clientID].connectedTo;
      strncpy(toSend, "refused:", BUF_SIZE-1);
      strncat(toSend, clients[clientID].name, sizeof(toSend) - strlen(toSend) - 1);
      write_client(clients[otherClientID].sock, toSend);
      clients[clientID].connectedTo = -1;
      clients[otherClientID].connectedTo = -1; 
   }else if(strcmp("cancel", token) == 0){
      int otherClientID = clients[clientID].connectedTo;
      strncpy(toSend, "cancel:", BUF_SIZE-1);
      strncat(toSend, clients[clientID].name, sizeof(toSend) - strlen(toSend) - 1);
      write_client(clients[otherClientID].sock, toSend);
      clients[clientID].connectedTo = -1;
      clients[otherClientID].connectedTo = -1; 
   }
   else if(strcmp("startGame", token) == 0)
   {
      int opponent_index = clients[clientID].connectedTo;
      GameState * gameState = &games[nbGames].state;
      initGameState(gameState);
      games[nbGames].player1 = clientID;
      games[nbGames].player2 = opponent_index;
      games[nbGames].state.currentPlayer = 0;
      clients[clientID].gameID = nbGames;
      clients[opponent_index].gameID = nbGames;
      nbGames++;
      write_client(clients[opponent_index].sock,"gameStarted:");
   }
   else if(strcmp("move", token) == 0)
   {
      printf("[MOVE] %s\n", buffer);
      int opponent_index = clients[clientID].connectedTo;
      int game_index = clients[clientID].gameID;
      int hole = atoi(strtok(NULL, "\0"));
      GameState * gameState = &games[game_index].state;
      *gameState = playTurn(gameState, hole);
      if(isFinished(gameState)){
         int winner = getWinner(gameState);
         if(winner == 0){
            write_client(clients[clientID].sock, "win:");
            write_client(clients[opponent_index].sock, "lose:");
         }else if(winner == 1){
            write_client(clients[clientID].sock, "lose:");
            write_client(clients[opponent_index].sock, "win:");
         }else{
            write_client(clients[clientID].sock, "draw:");
            write_client(clients[opponent_index].sock, "draw:");
         }
         clients[clientID].gameID = -1;
         clients[opponent_index].gameID = -1;
         nbGames--;
      }
      else{
         char game[BUF_SIZE] = "gameState:";
         serializeGameState(gameState, &game);
         write_client(clients[opponent_index].sock, game);
         write_client(clients[clientID].sock, game);
      }
   }
   else if(strcmp("quit", token) == 0)
   {
      if(clients[clientID].gameID != -1){
         int opponent_index = clients[clientID].connectedTo;
         clients[clientID].gameID = -1;
         clients[opponent_index].gameID = -1;
         nbGames--;
         write_client(clients[opponent_index].sock, "quit:");
      }
   }
   else if(strcmp("exit", token) == 0)
   {
      int opponent_index = clients[clientID].connectedTo;
      if(opponent_index != -1){
         clients[clientID].connectedTo = -1;
         clients[opponent_index].connectedTo = -1;
         write_client(clients[opponent_index].sock, "exit:");
      }
   }
   else if(strcmp("chat", token) == 0)
   {
      int opponent_index = clients[clientID].connectedTo;
      if(opponent_index != -1){
         token = strtok(NULL, "\0");
         strncpy(toSend, "chat:", BUF_SIZE - 1);
         strncat(toSend, clients[clientID].name, sizeof toSend - strlen(toSend) - 1);
         strncat(toSend, " : ", sizeof toSend - strlen(toSend) - 1);
         strncat(toSend, token, sizeof toSend - strlen(toSend) - 1);
         write_client(clients[opponent_index].sock, toSend);
      }
   }
   else if(strcmp("look_bio", token) == 0)
   {
      token = strtok(NULL, "\0");
      int otherClientID = atoi(token)-1;
      if(otherClientID>=actual || otherClientID<0){
         write_client(clients[clientID].sock, "error");
      }else{
         strncpy(toSend, "bio:", BUF_SIZE-1);
         strncat(toSend, clients[otherClientID].name, sizeof(toSend) - strlen(toSend) - 1);
         strncat(toSend, ",", sizeof(toSend) - strlen(toSend) - 1);
         strncat(toSend, clients[otherClientID].bio, sizeof(toSend) - strlen(toSend) - 1);
         write_client(clients[clientID].sock, toSend);
      }
   }
   else if(strcmp("getOwnBio", token) == 0)
   {
      strncpy(toSend, "bio:", BUF_SIZE-1);
      strncat(toSend, clients[clientID].bio, sizeof(toSend) - strlen(toSend) - 1);
      write_client(clients[clientID].sock, toSend);
   }
   else if(strcmp("edit_bio", token) == 0)
   {
      token = strtok(NULL, "\0");
      if(strlen(token)>BIO_SIZE+1){
         write_client(clients[clientID].sock, "error:");
      }else{
         strcpy(clients[clientID].bio, token);
         strncpy(toSend, "updated:", BUF_SIZE-1);
         strncat(toSend, clients[clientID].bio, sizeof(toSend) - strlen(toSend) - 1);
         write_client(clients[clientID].sock, toSend);
      }
   }
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

int getClientIndex(char * name, Client * clients, int actual){
   for(int i = 0; i < actual; i++){
      if(strcmp(name, clients[i].name) == 0){
         return i;
      }
   }
   return -1;
}

int main(int argc, char **argv)
{
   printf("[START] Server launched\n");

   init();

   app();

   end();

   return EXIT_SUCCESS;
}
