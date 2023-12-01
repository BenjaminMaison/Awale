#ifndef AWALE_H
#define AWALE_H

#define NUM_HOLES 6

#define BLACK "\033[1;30m"
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define MAGENTA "\033[1;35m"
#define CYAN "\033[1;36m"
#define WHITE "\033[1;37m"
#define RESET "\033[0m"

/* Board visualisation
    f e d c b a
    0 1 2 3 4 5 
    0 1 2 3 4 5
    A B C D E F
*/
// 0 is the first player, 1 is the second player
// 0 is the first row, 1 is the second row


typedef int Board[2][NUM_HOLES];

typedef struct GameState{
    Board board;
    int currentPlayer;
    int score[2];
} GameState;

typedef struct Game{
    GameState state;
    int player1;
    int player2;
    int* observers;
    int nbObservers;
} Game;

void initGameState(GameState* game);
GameState playTurn(GameState*game, int hole);
int isLegal(GameState* game, int hole);
int isOpponentStarving(GameState* game);
int* getLegalMoves(GameState* game);
int isFinished(GameState* game); 
int getWinner(GameState* game);
void displayGame(GameState* game, int player);

#endif /* guard */