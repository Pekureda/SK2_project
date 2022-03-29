#ifndef SERVER_HPP
#define SERVER_HPP

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <ctime>
#include <pthread.h>
#include <limits>

#include <sstream>
#include <list>
#include <memory>

#include "constants.hpp"
#include "communication.hpp"
#include "Game.hpp"

enum ServerErrorCodes {
    serverGameNotFound = INT32_MIN,
    serverRoomClosed
};

class Server {
    private:
        int server_socket_descriptor;

        void init(int argc, char* argv[]);
        void handleConnection(int connection_socket_descriptor, pthread_t* thread);
        std::list<std::unique_ptr<pthread_t>> connectionHandlingThreads;
        pthread_mutex_t games_mutex;
        pthread_mutex_t connectionHandling_mutex;
        std::vector<Game*> games;
    public:
        void start(int argc, char* argv[]);
        int connect(unsigned int game_id, ConnectionTypes type, int connection_socket_descriptor, std::string username);
        void removeGame(Game* game);
        static void* threadRoutine(void* arg);
        void stop();
};

#endif