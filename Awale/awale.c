#include "awale.h"
#include <stdio.h>
#include <stdlib.h>

//GameState game;

void init(GameState* game) 
{
    // Initialize the game state
    game->score[0] = 0;
    game->score[1] = 0;
    for (int i = 0; i < NUM_HOLES; i++) {
        game->board[0][i] = 4;
        game->board[1][i] = 4;
    }
    game->currentPlayer = 0;
}

/**
 * @brief Check if the game is finished
 * 
 * @return int - 1 if the game is finished, 0 otherwise
 */
int isFinished(GameState* game)
{
    if(game->score[0] >= 25 || game->score[1] >= 25) {
        return 1;
    } 
    int* legalMoves = getLegalMoves(game);
    if(legalMoves == NULL) {
        return 1;
    }
    else 
    {
        free(legalMoves);
        return 0;
    }
}

/**
 * @brief Check if a move is legal
 * 
 * @param hole the chosen hole
 * @return int - 1 if the move is legal, 0 otherwise
 */
int isLegal(GameState* game,int hole)
{
    // Check if the chosen hole is valid and contains stones
    if (hole < 0 || hole >= NUM_HOLES || game->board[game->currentPlayer][hole] == 0) {
        return 0;
    }

    GameState newState = playTurn(game, hole);
    if(isOpponentStarving(&newState)) 
    {
        return 0;
    }
    return 1;
}

/**
 * @brief Get an array of legal moves. If not null, the array must be freed after use
 * @return int* - an array of legal moves, NULL if there are no legal moves
 */
int* getLegalMoves(GameState* game)
{
    // Return an array of legal moves
    int* legalMoves = malloc(NUM_HOLES * sizeof(int));
    int numLegalMoves = 0;
    for(int i = 0; i < NUM_HOLES; i++) {
        if(isLegal(game, i)) {
            legalMoves[numLegalMoves] = i;
            numLegalMoves++;
        }
    }
    if(numLegalMoves == 0) {
        free(legalMoves);
        return NULL;
    }
    legalMoves = realloc(legalMoves, numLegalMoves * sizeof(int));
    return legalMoves;
}

/**
 * @brief Check if the opponent is starving (all holes are empty)
 * 
 * @param game the game state
 * @return int - 1 if the opponent is starving, 0 otherwise
 */
int isOpponentStarving(GameState* game)
{
    // Check if the opponent is starving
    int player = 1 - game->currentPlayer;
    for(int i = 0; i < NUM_HOLES; i++) {
        if(game->board[player][i] > 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Play a turn. The chosen hole is emptied and its stones are distributed counterclockwise. 
 * @param hole must be a legal move
 * @return GameState - the new game state
 */
GameState playTurn(GameState* game, int hole) {
    GameState newGame = *game;

    // Pick up all the stones from the chosen hole
    if(newGame.currentPlayer == 0) {
        hole = NUM_HOLES - 1 - hole;
    }
    int stones = newGame.board[newGame.currentPlayer][hole];
    newGame.board[newGame.currentPlayer][hole] = 0;

    int player = newGame.currentPlayer;

    // Distribute the stones counterclockwise
    while(stones > 0)
    {
        player ? hole++ : hole--;
        if(hole == NUM_HOLES) {
            hole = NUM_HOLES - 1;
            player = 1 - player;
        }
        else if(hole == -1) {
            hole = 0;
            player = 1 - player;
        }
        newGame.board[player][hole]++;
        stones--;
    }

    // Claim stones from opponent's holes
    while (player != newGame.currentPlayer && (newGame.board[player][hole] == 2 || newGame.board[player][hole] == 3)) {
        newGame.score[newGame.currentPlayer] += newGame.board[player][hole];
        newGame.board[player][hole] = 0;
        player ? hole-- : hole++;
        if(hole == NUM_HOLES) {
            hole = NUM_HOLES - 1;
            player = 1 - player;
        }
        else if(hole == -1) {
            hole = 0;
            player = 1 - player;
        }
    }

    // Switch to the other player
    newGame.currentPlayer = 1 - newGame.currentPlayer;
    return newGame;
}

void display(GameState *game)
{
    int currentPlayer = game->currentPlayer;
    printf("Player %d's turn\n", currentPlayer);
    printf("Score: %d - %d\n", game->score[0], game->score[1]);
    
    // Display the board
    char topLabel = (currentPlayer == 0) ? 'A' : 'a';
    char bottomLabel = (currentPlayer == 0) ? 'a' : 'A';

    // Print column labels
    printf("       ");
    for(char label = topLabel + NUM_HOLES - 1; label >= topLabel; label--) {
        printf("%c         ", label);
    }
    printf("\n");

    // Print separator
    printf("  |");
    for(int i = 0; i < NUM_HOLES; i++) {
        printf("---------");
    }
    printf("-----|\n");

    // Print upper part of the board
    for(int i = (currentPlayer == 0) ? (NUM_HOLES - 1) : 0; 
            (currentPlayer == 0) ? (i >= 0) : (i < NUM_HOLES); 
            (currentPlayer == 0) ? (i--) : (i++)) {
        printf("  |    %d  ", game->board[1 - currentPlayer][i]);
    }

    printf("  |\n");
    // Print separator
    printf("  |");
    for(int i = 0; i < NUM_HOLES; i++) {
        printf("---------");
    }
    printf("-----|\n");

    // Print lower part of the board
    for(int i = (currentPlayer == 0) ? (NUM_HOLES - 1) : 0; 
            (currentPlayer == 0) ? (i >= 0) : (i < NUM_HOLES); 
            (currentPlayer == 0) ? (i--) : (i++)) {
        printf("  |    %d  ", game->board[currentPlayer][i]);
    }
    printf("  |\n");

    // Print separator
    printf("  |");
    for(int i = 0; i < NUM_HOLES; i++) {
        printf("---------");
    }
    printf("-----|\n");

    // Print column labels
    printf("       ");
    for(char label = bottomLabel; label < bottomLabel + NUM_HOLES; label++) {
        printf("%c         ", label);
    }
    
    printf("\n\n"); // Add extra line for separation
}

void clear()
{
    // Clear the terminal
    printf("\033[2J\033[1;1H");
}

void refresh()
{
    // Refresh the terminal
    printf("\033[0;0H");
}

int main()
{
    GameState game;
    init(&game);
    clear();
    display(&game);
    int hole;
    while(!isFinished(&game)) {
        printf("\nPlayer %d, choose a hole: ", game.currentPlayer);
        scanf("%d", &hole);
        game = playTurn(&game, hole);
        refresh();
        display(&game);
    }
    printf("Player %d wins!\n", game.score[0] > game.score[1] ? 0 : 1);
    return 0;
}