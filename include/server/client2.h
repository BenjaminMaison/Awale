#ifndef CLIENT_H
#define CLIENT_H

#include "server2.h"

typedef struct
{
   SOCKET sock;
   char name[BUF_SIZE];
   int connectedTo;
   int gameID;
   char bio[BIO_SIZE];
}Client;

#endif /* guard */
