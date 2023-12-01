#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../../include/client/client2.h"

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#endif

enum State {MENU, CONNECTION, GAME, END, INVITATION, CONNECTED, LOOK_BIO, EDIT_BIO, CHOOSE_GAME, WATCH_GAME};
typedef enum State State;

GameState gameState;
int player = 0;

State state = MENU;
int waiting = 0;

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

static void app(const char *address, const char *name)
{
   SOCKET sock = init_connection(address);
   char buffer[BUF_SIZE];

   fd_set rdfs;

   /* send our name */
   write_server(sock, name);

   clear();
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
   #ifdef DEBUG
   debug("[READ SERVER] %s\n", buffer);
   #endif
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
   #ifdef DEBUG
   debug("[CLIENT COMMAND] %s\n", buffer);
   #endif
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void user_update(SOCKET sock,char* buffer)
{
   char toSend[BUF_SIZE];
   switch (state) {
      case MENU:
            if(strcmp("1", buffer) == 0){
               write_server(sock, "getListPlayers:1");
            }
            else if(strcmp("2", buffer) == 0){
               write_server(sock, "getListPlayers:2");
            }
            else if(strcmp("3", buffer) == 0){
               write_server(sock, "getOwnBio:");
            }
            else if(strcmp("4", buffer) == 0) {
               write_server(sock, "getListGames:");
            }
            break;
      case LOOK_BIO:
         if(strcmp("quit", buffer) == 0){
            clear();
            menu_initial();
         }else{
            strcpy(toSend, "look_bio:");
            strcat(toSend, buffer);
            write_server(sock, toSend);
         }
         break;
      case EDIT_BIO:
         if(strcmp("quit", buffer) == 0){
            clear();
            menu_initial();
         }else{
            strcpy(toSend, "edit_bio:");
            strcat(toSend, buffer);
            write_server(sock, toSend);
         }
         break;
      case CHOOSE_GAME:
         if(strcmp("quit", buffer) == 0){
            clear();
            menu_initial();
         }else{
            strcpy(toSend, "chooseGame:");
            strcat(toSend, buffer);
            write_server(sock, toSend);
         }
         break;
      case CONNECTION:
         if(strcmp("quit", buffer) == 0){
            clear();
            menu_initial();
         }else{
            strcpy(toSend, "connectToPlayer:");
            strcat(toSend, buffer);
            write_server(sock, toSend);
         }
         break;
      case INVITATION:
         if(!waiting && strcmp("1", buffer) == 0){
            write_server(sock, "accept:");
            clear();
            menu_connected();
         }else if(!waiting && strcmp("2", buffer) == 0){
            write_server(sock, "refuse:");
            clear();
            menu_initial();
         }else if(waiting && strcmp("cancel", buffer) == 0){
            write_server(sock, "cancel:");
            clear();
            menu_initial();
         }
         break;
      case CONNECTED:
         if(strcmp("chat", strtok(buffer, " ")) == 0){
            strcpy(toSend, "chat:");
            strcat(toSend, strtok(NULL, "\0"));
            write_server(sock, toSend);
         }
         else if(strcmp("1", buffer) == 0){
            write_server(sock, "startGame:");
            clear();
            printf("You started a game.\n");
            initGameState(&gameState);
            player = 0;
            menu_game();
         }else if(strcmp("exit", buffer) == 0){
            write_server(sock, "exit:");
            clear();
            menu_initial();
         }
         break;
      case GAME:
         if(strcmp("chat", strtok(buffer, " ")) == 0){
            strcpy(toSend, "chat:");
            strcat(toSend, strtok(NULL, "\0"));
            write_server(sock, toSend);
         }
         else if(strlen(buffer) == 1 && ((buffer[0] >= 'a' && buffer[0] <= 'f') || (buffer[0] >= 'A' && buffer[0] <= 'F'))){
            int hole = letterToHole(buffer[0]);
            if(gameState.currentPlayer != player){
               printf("It's not your turn !\n");
            }
            else if(!isLegal(&gameState, hole)){
               printf("Illegal move !\n");
            }
            else
            {
               char holeChar[2];
               sprintf(holeChar, "%d", hole);
               strcpy(toSend, "move:");
               strcat(toSend, holeChar);
               write_server(sock, toSend);
            }
         }
         else if(strcmp("quit", buffer) == 0){
            write_server(sock, "quit:");
            clear();
            menu_connected();
         }
         else {
            printf("Please enter a letter between a (or A) and f (or F). Enter 'quit' to exit the game\n");
         }
         break;
      case WATCH_GAME:
         if(strcmp("quit", buffer) == 0){
            write_server(sock, "stop:");
            clear();
            menu_initial();
         }
         break;
      default:
         break;
   }
}

static void server_update(SOCKET sock, char* buffer)
{
   // copy buffer
   char temp[BUF_SIZE];
   strcpy(temp, buffer);
   char* cmd = strtok(temp, ":");
   #ifdef DEBUG
   debug("[SERVER COMMAND] %s\n", buffer);
   #endif
   switch (state) {
      case MENU:
         if(strcmp("listPlayers", cmd) == 0){
            clear();
            int bio = atoi(strtok(NULL, ","));
            displayPlayers(bio, strtok(NULL, "\0"));
         }else if(strcmp("invite", cmd) == 0){
            clear();
            menu_invitation(strtok(NULL, "\0"));
         }else if(strcmp("bio", cmd) == 0){
            clear();
            char* bio = strtok(NULL, "\0");
            printf("Your bio is: %s\n", bio);
            menu_edit_bio();
         } else if(strcmp("listGames", cmd) == 0){
            clear();
            printf("List of games:\n");
            displayGames(strtok(NULL, "\0"));
         }
         break;
      case LOOK_BIO:
         if(strcmp("error", cmd) == 0){
            printf("Please enter a correct player number\n");
         }else if(strcmp("bio", cmd) == 0){
            char* name = strtok(NULL, ",");
            char* bio = strtok(NULL, "\0");
            printf("%s's bio : %s\n", name, bio);
            menu_look_bio();
         }
         break;
      case EDIT_BIO:
         if(strcmp("error", cmd) == 0){
            printf("Please enter a shorter bio\n");
         }
         else if(strcmp("updated", cmd) == 0){
            clear();
            printf("Your bio has been succesfully updated ! \n");
            char* bio = strtok(NULL, "\0");
            printf("Your new bio is: %s\n", bio);
            menu_initial();
         }
         break;
      case CHOOSE_GAME:
         if(strcmp("error", cmd) == 0){
            printf("Please enter a correct game number\n");
         }else if(strcmp("gameState", cmd) == 0){
            clear();
            //strcpy(player1, strtok(NULL, ","));
            //strcpy(player2, strtok(NULL, ","));
            //printf("You are now watching the game between %s and %s\n", player1, player2);
            printf("You are now watching the game\n");
            initGameState(&gameState);
            deserializeGameState(buffer, &gameState);
            player = -1;
            displayGame(&gameState, player);
            menu_watch_game();
         }
         break;
      case CONNECTION:
         if(strcmp("error", cmd) == 0){
            printf("Please enter a correct player number\n");
         }else if(strcmp("sent", cmd) == 0){
            state = INVITATION;
            waiting = 1;
            printf("Invitation sent ! Enter 'cancel' to cancel\n");
         }else if(strcmp("invite", cmd) == 0){
            clear();
            menu_invitation(strtok(NULL, "\0"));
         }
         break;
      case INVITATION:
         if(strcmp("accepted", cmd) == 0){
            waiting = 0;
            clear();
            printf("%s accepted your invitation !\n", strtok(NULL, "\0"));
            menu_connected();
         }else if(strcmp("refused", cmd) == 0){
            waiting = 0;
            clear();
            printf("%s refused your invitation...\n", strtok(NULL, "\0"));
            menu_initial();
         }else if(strcmp("cancel", cmd) == 0){
            waiting = 0;
            clear();
            printf("%s canceled the invitation...\n", strtok(NULL, "\0"));
            menu_initial();
         }else if(strcmp("disconnected", cmd) == 0){
            waiting = 0;
            clear();
            printf("%s disconnected...\n", strtok(NULL, "\0"));
            menu_initial();
         }
         break;
      case CONNECTED:
         if(strcmp("gameStarted", cmd) == 0){
            clear();
            printf("Your opponent has started a game.\n");
            initGameState(&gameState);
            player = 1;
            menu_game();
         }else if(strcmp("disconnected", cmd) == 0){
            clear();
            printf("%s disconnected...\n", strtok(NULL, "\0"));
            menu_initial();
         }else if(strcmp("exit", cmd) == 0){
            clear();
            printf("Your opponent left the game !\n");
            menu_initial();
         }else if(strcmp("chat", cmd) == 0){
            cmd = strtok(NULL, "\0");
            printf("%s\n", cmd);
         }
         break;
      case WATCH_GAME:
         if(strcmp("gameState", cmd) == 0) {
            deserializeGameState(buffer, &gameState);
            clear();
            displayGame(&gameState, player);
         }
         else if(strcmp("win", cmd) == 0)
         {
            printf("Player 0 won !\n");
            menu_initial();
         }
         else if(strcmp("lose", cmd) == 0)
         {
            printf("Player 1 won !\n");
            menu_initial();
         }
         else if(strcmp("draw", cmd) == 0)
         {
            printf("It's a draw !\n");
            menu_initial();
         }
         else if(strcmp("quit", cmd) == 0)
         {
            clear();
            printf("One of the player left the game !\n");
            menu_initial();
         }
         else if(strcmp("disconnected", cmd) == 0){
            clear();
            printf("%s disconnected...\n", strtok(NULL, "\0"));
            menu_initial();
         }
         break;
      case GAME:
         if(strcmp("gameState", cmd) == 0){
            deserializeGameState(buffer, &gameState);
            clear();
            displayGame(&gameState, player);
         }
         else if(strcmp("win", cmd) == 0)
         {
            printf("You won !\n");
            menu_connected();
         }
         else if(strcmp("lose", cmd) == 0)
         {
            printf("You lost !\n");
            menu_connected();
         }
         else if(strcmp("draw", cmd) == 0)
         {
            printf("It's a draw !\n");
            menu_connected();
         }
         else if(strcmp("quit", cmd) == 0)
         {
            clear();
            printf("Your opponent quit the game !\n");
            menu_connected();
         }
         else if(strcmp("chat", cmd) == 0)
         {
            cmd = strtok(NULL, "\0");
            printf("%s\n", cmd);
         }
         else if(strcmp("disconnected", cmd) == 0){
            clear();
            printf("%s disconnected...\n", strtok(NULL, "\0"));
            menu_initial();
         }
         break;
      default:
         break;
   }
}

static void menu_initial()
{
   state = MENU;
   printf("Welcome to the Awale game!\n\n");

   printf("Menu :\n");
   printf("1. Connect to an other player\n");
   printf("2. See the other players bio\n");
   printf("3. Edit your own bio\n");
   printf("4. Watch a game\n");
}

static void menu_invitation(char* name)
{
   state = INVITATION;
   printf("%s sent you an invitation\n", name);
   printf("1 - Accept\n");
   printf("2 - Refuse\n");
}

static void menu_choose_game()
{
   state = CHOOSE_GAME;
   printf("Enter the number of a game or 'quit' to exit this menu : \n");
}

static void menu_watch_game()
{
   state = WATCH_GAME;
   printf("Enter 'quit' to exit the game\n");
}

static void menu_connection()
{
   state = CONNECTION;
   printf("Enter the number of a player or 'quit' to exit this menu : \n");
}

static void menu_look_bio()
{
   state = LOOK_BIO;
   printf("Enter the number of a player or 'quit' to exit this menu : \n");
}

static void menu_edit_bio()
{
   state = EDIT_BIO;
   printf("Enter your new bio or 'quit' to exit this menu : \n");
}

static void menu_connected()
{
   state = CONNECTED;
   printf("Write 'chat + message' to write in the chat\n");
   printf("1. Start the game\n");
   printf("Enter 'exit' to disconnect from this player\n");
}

static void menu_game()
{
   state = GAME;
   displayGame(&gameState, player);
}

static void displayPlayers(int bio, char* buffer){
   char* token = strtok(buffer, ",");
   int i = 1;
   while(token != NULL){
      printf("Player %d: %s, ", i, token);
      token = strtok(NULL, ",");
      printf("%s\n", token);
      token = strtok(NULL, ",");
      i++;
   }
   if(bio == 1){
      menu_connection();
   }else{
      menu_look_bio();
   }

}

void displayGames(char* buffer){
   char* token = strtok(buffer, ",");
   int i = 1;
   while(token != NULL){
      printf("Game %d: %s, ", i, token);
      token = strtok(NULL, ",");
      printf("%s\n", token);
      token = strtok(NULL, ",");
      i++;
   }
   menu_choose_game();
}

/**
 * @brief Deserialize a game state from a buffer
 * 
 * @param buffer 
 * @param gameState 
 * @return int - EXIT_SUCCESS if success, EXIT_FAILURE otherwise
 */
static int deserializeGameState(char* buffer, GameState* gameState) {
   const char * separators = "\n :";
   char* token = strtok(buffer, separators);
   if(strcmp("gameState", token) != 0){
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

int letterToHole(char letter)
{
    if(letter >= 'A' && letter <= 'F') {
        return letter - 'A';
    }
    else if(letter >= 'a' && letter <= 'f') {
        return letter - 'a';
    }
    else {
        return -1;
    }
}

static void clear()  
{
   // Clear the terminal
   printf("\033[2J\033[1;1H");
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
