#ifndef REQUEST_H
#define REQUEST_H

enum Action {LIST_PLAYERS, CONNECT, DISCONNECT, START_GAME, MOVE, CHAT, QUIT};
const char* actions[] = {"getlistPlayers", "connect", "disconnect", "startGame", "game", "move", "chat", "quit"};

#endif /* guard */