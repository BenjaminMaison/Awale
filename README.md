# Awale

## How to build
Simply run the following command in the root folder of the project :
```shell
make all
```

This will build the client.exe and server.exe executables.

## How to play
First, run the server :
```Shell
./server.exe
```

Then, run as many clients as you want with the following command. Replace **server_ip_address** by your server ip adress and **pseudo** by the pseudo of the client. 
```Shell
./client.exe server_ip_address pseudo
```

Here are the possibilities for the client :
- clients can modify their bio and see other people's bio
- clients can connect to other players online, and once connected, they can start a game
- when connected or when playing a game, players can chat with their
- players can also watch other people's games. 