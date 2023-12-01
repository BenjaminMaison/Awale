#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include "../../include/server/server2.h"

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

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
   srand(time(NULL));

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
                     if(client.gameID != -1){
                        int game_index = client.gameID;
                        for(int i = 0; i < games[game_index].nbObservers; i++){
                           write_client(clients[games[game_index].observers[i]].sock, toSend);
                        }
                        free(games[game_index].observers);
                        games[game_index].nbObservers = 0;
                        clients[client.connectedTo].gameID = -1;
                        nbGames--;
                     }
                  }
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
               }
               else
               {
                  action(buffer, clients, actual, i);
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
   // first save the clients array
   Client temp_clients[MAX_CLIENTS];
   memcpy(temp_clients, clients, MAX_CLIENTS*sizeof(Client));

   for(int i = to_remove+1; i < *actual; i++){
      if(temp_clients[i].connectedTo != -1)
      {
         int opponent = temp_clients[i].connectedTo;
         clients[opponent].connectedTo--;
      }      
   }
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
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
   printf("[CLIENT %d RESPONSE] %s\n", sock, buffer);
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void action(char *buffer, Client *clients, int actual, int clientID){
   char toSend[BUF_SIZE];
   SOCKET sock = clients[clientID].sock;
   char* token = strtok(buffer, ":");
   #ifdef DEBUG
   debug("[ACTION]: %s", token);
   #endif
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
      int randomInt = rand() % 2;
      int client;
      int opponent;
      if(randomInt == 0)
      {
         printf("Player 0 starts\n");
         client = 0;
         opponent = 1;
      } else {
         printf("Player 1 starts\n");
         client = 1;
         opponent = 0;
      }
      games[nbGames].player1 = client;
      games[nbGames].player2 = opponent;
      char client_id[2];
      sprintf(client_id, "%d", client);
      char opponent_id[2];
      sprintf(opponent_id, "%d", opponent);

      games[nbGames].state.currentPlayer = 0;
      games->observers = NULL;
      clients[clientID].gameID = nbGames;
      clients[opponent_index].gameID = nbGames;
      nbGames++;
      char clientMsg[BUF_SIZE];
      strncpy(clientMsg, "gameStarted:", BUF_SIZE-1);
      strncat(clientMsg, client_id, sizeof(toSend) - strlen(clientMsg) - 1);
      write_client(clients[opponent_index].sock, clientMsg);
      
      char opponentMsg[BUF_SIZE];
      strncpy(opponentMsg, "gameStarted:", BUF_SIZE-1);
      strncat(opponentMsg, opponent_id, sizeof(toSend) - strlen(opponentMsg) - 1);
      write_client(clients[clientID].sock, opponentMsg);
   }
   else if(strcmp("move", token) == 0)
   {
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
            for(int i = 0; i < games[game_index].nbObservers; i++){
               write_client(clients[games[game_index].observers[i]].sock, "win:");
            }
         }else if(winner == 1){
            write_client(clients[clientID].sock, "lose:");
            write_client(clients[opponent_index].sock, "win:");
            for(int i = 0; i < games[game_index].nbObservers; i++){
               write_client(clients[games[game_index].observers[i]].sock, "lose:");
            }
         }else{
            write_client(clients[clientID].sock, "draw:");
            write_client(clients[opponent_index].sock, "draw:");
            for(int i = 0; i < games[game_index].nbObservers; i++){
               write_client(clients[games[game_index].observers[i]].sock, "draw:");
            }
         }
         clients[clientID].gameID = -1;
         clients[opponent_index].gameID = -1;
         nbGames--;
      }
      else{
         strncpy(toSend, "gameState:", BUF_SIZE-1);
         serializeGameState(&games[game_index].state, toSend);
         write_client(clients[opponent_index].sock, toSend);
         write_client(clients[clientID].sock, toSend);
         for(int i = 0; i < games[game_index].nbObservers; i++){
            write_client(clients[games[game_index].observers[i]].sock, toSend);
         }
      }
   }
   else if(strcmp("quit", token) == 0)
   {
      int game_index = clients[clientID].gameID;
      if(game_index != -1){
         int opponent_index = clients[clientID].connectedTo;
         clients[clientID].gameID = -1;
         clients[opponent_index].gameID = -1;
         write_client(clients[opponent_index].sock, "quit:");
         #ifdef DEBUG
         debug("nbObservers: %d", games[game_index].nbObservers);
         #endif
         for(int i = 0; i < games[game_index].nbObservers; i++){
            write_client(clients[games[game_index].observers[i]].sock, "quit:");
         }
         free(games[game_index].observers); 
         games[game_index].nbObservers = 0;
         nbGames--;
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
         strncat(toSend, "[", sizeof toSend - strlen(toSend) - 1);
         strncat(toSend, clients[clientID].name, sizeof toSend - strlen(toSend) - 1);
         strncat(toSend, "]", sizeof toSend - strlen(toSend) - 1);
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
   else if(strcmp("getListGames", token) == 0)
   {
      strncpy(toSend, "listGames:", BUF_SIZE-1);
      for(int i = 0; i < nbGames; i++){
         strncat(toSend, clients[games[i].player1].name, sizeof(toSend) - strlen(toSend) - 1);
         strncat(toSend, ",", sizeof(toSend) - strlen(toSend) - 1);
         strncat(toSend, clients[games[i].player2].name, sizeof(toSend) - strlen(toSend) - 1);
         strncat(toSend, ",", sizeof(toSend) - strlen(toSend) - 1);
      }
      write_client(clients[clientID].sock, toSend);
   }
   else if(strcmp("chooseGame", token) == 0)
   {
      int game_index = atoi(strtok(NULL, "\0"))-1;
      if(game_index>=nbGames || game_index<0){
         write_client(clients[clientID].sock, "error");
      }else{
         strncpy(toSend, "gameState:", BUF_SIZE-1);
         serializeGameState(&games[game_index].state, toSend);
         write_client(clients[clientID].sock, toSend);
         if(games[game_index].nbObservers > 0){
            games[game_index].observers = realloc(games[game_index].observers, (games[game_index].nbObservers+1)*sizeof(int));
            games[game_index].observers[games[game_index].nbObservers] = clientID;
         }
         else{
            games[game_index].observers = malloc(sizeof(int));
            games[game_index].observers[0] = clientID;
         }
         clients[clientID].gameID = game_index;
         games[game_index].nbObservers++;
      }
   }
   else if(strcmp("stop", token) == 0){
      // case where the client is an observer
      int game_index = clients[clientID].gameID;
      if(game_index != -1){
         int* observers = games[game_index].observers;
         int nbObservers = games[game_index].nbObservers;
         int i = 0;
         for(i = 0; i < nbObservers; i++){
            if(observers[i] == clientID){
               break;
            }
         }
         if(i < nbObservers && nbObservers > 1){
            memmove(observers + i, observers + i + 1, (nbObservers - i - 1) * sizeof(int));
         }
         games[game_index].nbObservers--;
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
static int serializeGameState(const GameState* gameState, char* buffer){
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
