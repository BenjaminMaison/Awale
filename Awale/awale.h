#define NUM_HOLES 6

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

void init();
GameState playTurn(int hole);
int isLegal();
int isFinished(); 