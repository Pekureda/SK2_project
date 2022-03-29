#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <string>
#include <pthread.h>

#include "communication.hpp"
#include "constants.hpp"

class Game;

class Connection
{
private:
    ConnectionTypes type;
    int connection_socket_descriptor;
    std::string username;
    pthread_t thread;
    Game* gameptr;
    bool disconnect = false;
    //unsigned int game_id;
public:
    Connection(ConnectionTypes type, int connection_socket_descriptor, std::string username) : type(type),
                                                                                              connection_socket_descriptor(connection_socket_descriptor),
                                                                                              username(username)
    { }

    Connection(const Connection& conn);
    ~Connection();

    ConnectionTypes getType();
    int getConnectionSocketDescriptor();
    std::string getUsername();

    void setGame(Game* gameptr);

    static void* threadRoutine(void* arg);

    void start();

    void playerBehaviour();
    void hostBehaviour();
    
    void terminateConnection();
};

#endif