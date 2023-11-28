#include "awale.h"
#include <stdio.h>
GameState game;

void init() 
{
    // Initialize the game state
    game.score[0] = 0;
    game.score[1] = 0;
    for (int i = 0; i < NUM_HOLES; i++) {
        game.board[0][i] = 4;
        game.board[1][i] = 4;
    }
    game.currentPlayer = 0;
}

int isLegal(int hole)
{
    // Check if the chosen hole is valid and contains stones
    if (hole < 0 || hole >= NUM_HOLES || game.board[game.currentPlayer][hole] == 0) {
        return 0;
    }
    return 1;
}

GameState playTurn(int hole) {
    printf("Player %d plays hole %d\n", game.currentPlayer, hole);
    if(!isLegal(hole)) {
        return game;
    }

    // Pick up all the stones from the chosen hole
    if(game.currentPlayer == 0) {
        hole = NUM_HOLES - 1 - hole;
    }
    int stones = game.board[game.currentPlayer][hole];
    game.board[game.currentPlayer][hole] = 0;

    int player = game.currentPlayer;

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
        game.board[player][hole]++;
        stones--;
    }

    // Claim stones from opponent's holes
    while (player != game.currentPlayer && (game.board[player][hole] == 2 || game.board[player][hole] == 3)) {
        game.score[game.currentPlayer] += game.board[player][hole];
        game.board[player][hole] = 0;
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
    game.currentPlayer = 1 - game.currentPlayer;

    return game;
}

void display()
{
    int player = game.currentPlayer;
    printf("Player %d's turn\n", player);
    printf("Score: %d - %d\n", game.score[0], game.score[1]);
    
    // Display the board
    printf("    ");
    if(player == 0)
    {
        for(char c = 'A' + NUM_HOLES -1 ; c >= 'A'; c--) {
            printf("%c ", c);
        }
        printf("\n    ");
        for(int i = NUM_HOLES -1 ; i >= 0; i--) {
            printf("%d ", game.board[1][i]);
        }
        printf("\n    ");
        for(int i = NUM_HOLES -1 ; i >= 0; i--) {
            printf("%d ", game.board[0][i]);
        }
        printf("\n    ");
        for(char c = 'a'; c < 'a' + NUM_HOLES; c++) {
            printf("%c ", c);
        }
    }
    else
    {
        for(char c = 'a' + NUM_HOLES -1 ; c >= 'a'; c--) {
            printf("%c ", c);
        }
        printf("\n    ");
        for(int i = 0; i <  NUM_HOLES; i++) {
            printf("%d ", game.board[0][i]);
        }
        printf("\n    ");
        for(int i = 0; i < NUM_HOLES; i++) {
            printf("%d ", game.board[1][i]);
        }
        printf("\n    ");
        for(char c = 'A'; c < 'A' + NUM_HOLES; c++) {
            printf("%c ", c);
        }
    }
    printf("\n");

}

int isFinished()
{
    if(game.score[0] >= 25 || game.score[1] >= 25) {
        return 1;
    } 
    else
    {
        return 0;
    }
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
    init();
    clear();
    display();
    int hole;
    while(!isFinished()) {
        printf("\nPlayer %d, choose a hole: ", game.currentPlayer);
        scanf("%d", &hole);
        playTurn(hole);
        clear();
        display();
    }
    printf("Player %d wins!\n", game.score[0] > game.score[1] ? 0 : 1);
    return 0;
}