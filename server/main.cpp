#include "Server.hpp"

Server* s = new Server();

void stopServer(int signum) {
    s->stop();
    delete s;
}

int main(int argc, char* argv[]) {
    signal(SIGINT, stopServer);
    //signal(SIGPIPE, SIG_IGN); // ignoruje sygnały o zamkniętym socketcie
    s->start(argc, argv);
    
    return 0;
}