#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#define SERVER_PORT 1234
#define QUEUE_SIZE 5

#define BUF_SIZE 1024

// 4 byte uint as ascii: A=0,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P ## Do poprawy. Właściwie ogranicza do max 15 znaków
#define DEBUG true

enum class ConnectionTypes {
    host,
    player
};

#endif