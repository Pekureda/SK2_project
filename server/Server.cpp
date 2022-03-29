#include "Server.hpp"

#include <ctime>
#include <sys/time.h>

void Server::init(int argc, char *argv[])
{
    games.reserve(1);
    games_mutex = PTHREAD_MUTEX_INITIALIZER;

    int bind_result;
    int listen_result;
    char reuse_addr_val = 1;
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(SERVER_PORT);

    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_descriptor < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda..\n", argv[0]);
        exit(1);
    }
    setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse_addr_val, sizeof(reuse_addr_val));

    bind_result = bind(server_socket_descriptor, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in));
    if (bind_result < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie dowiązania adresu IP i numeru portu do gniazda.\n", argv[0]);
        exit(1);
    }

    listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
    if (listen_result < 0)
    {
        fprintf(stderr, "%s: Błąd przy próbie ustawienia wielkości kolejki.\n", argv[0]);
        exit(1);
    }
}

struct connHandleStruct {
    pthread_t* thread;
    Server* serv;
    int csd;
};

void* Server::threadRoutine(void* arg) {
    pthread_detach(pthread_self());
    connHandleStruct* temp = static_cast<connHandleStruct*>(arg);
    temp->serv->handleConnection(temp->csd, temp->thread);

    //delete temp->thread; // ???
    pthread_exit(NULL);
}

void Server::start(int argc, char *argv[])
{
    init(argc, argv);

    int connection_socket_descriptor;

    while (true)
    {
        printf("Listening...\n");
        connection_socket_descriptor = accept(server_socket_descriptor, NULL, NULL);

        if (connection_socket_descriptor < 0)
        {
            fprintf(stderr, "%s: Błąd przy próbie utworzenia gniazda dla połączenia.\n", argv[0]);
            exit(1);
        }
        printf("Handling incoming connection!\n");
        
        pthread_mutex_lock(&connectionHandling_mutex);

        connectionHandlingThreads.push_back(std::make_unique<pthread_t>());
        connHandleStruct temp = {connectionHandlingThreads.back().get(), this, connection_socket_descriptor};
        pthread_create(connectionHandlingThreads.back().get(), NULL, threadRoutine, (void*)&temp);
        
        pthread_mutex_unlock(&connectionHandling_mutex);
    }
}

template<class T> class gameComp {
    private:
        T val;
    public:
        gameComp(T val) : val(val) { }

        bool operator()(const Game* gm) {
            return (gm->getGameId() == val);
        }
};

int Server::connect(unsigned int game_id, ConnectionTypes type, int connection_socket_descriptor, std::string username) {
    int ret_val = 0;

    pthread_mutex_lock(&games_mutex);

    std::vector<Game*>::iterator it = std::find_if(games.begin(), games.end(), gameComp<unsigned int>(game_id));

    if (it == games.end() || games.empty()) ret_val = ServerErrorCodes::serverGameNotFound;
    else if((*it)->addPlayer(new Connection(type, connection_socket_descriptor, username)) == GameErrorCodes::gameRoomClosed) ret_val = ServerErrorCodes::serverRoomClosed;

    pthread_mutex_unlock(&games_mutex);

    return ret_val;
}

void Server::handleConnection(int connection_socket_descriptor, pthread_t* thread) {

    std::stringstream ss(cmn::readData(connection_socket_descriptor));
    int code = 0;

    std::string user_type, username;
    ss >> user_type;
    
    // handling players
    if (user_type == "player") {
        std::getline(ss, username);

        username.erase(std::remove_if(username.begin(), username.end(), [](char c) {return  !std::isprint(c); }), username.end());
        if (username.empty()) {
            cmn::sendData(connection_socket_descriptor, "error username\n");
            shutdown(connection_socket_descriptor, SHUT_RDWR);
            return;
        }
        username = username.substr(1, username.length() - 1); // removing leading whitespace from getline
        ss = std::stringstream(cmn::readData(connection_socket_descriptor));
        std::string tag, game_id;
        ss >> tag >> game_id;
        try {
            if (tag == "gid"){
                if((code = connect(std::stoul(game_id), ConnectionTypes::player, connection_socket_descriptor, username))) {
                    std::string message;
                    if (code == ServerErrorCodes::serverGameNotFound) message = "gc " + /*game_id +*/ std::string("nosuchroom\n");
                    else if (code == ServerErrorCodes::serverRoomClosed) message = "gc " + /*game_id +*/ std::string("roomclosed\n");
                    cmn::sendData(connection_socket_descriptor, message);
                    shutdown(connection_socket_descriptor, SHUT_RDWR);
                }
            }
            else {
                cmn::sendData(connection_socket_descriptor, "expected gid\n");
                shutdown(connection_socket_descriptor, SHUT_RDWR);
            }
        } catch (std::invalid_argument& e){
            cmn::sendData(connection_socket_descriptor, "error gid " + game_id + '\n');
            shutdown(connection_socket_descriptor, SHUT_RDWR);
        }
    }

    // handling hosts
    else if (user_type == "host") {
        pthread_mutex_lock(&games_mutex);

        games.push_back(new Game(this, new Connection(ConnectionTypes::host, connection_socket_descriptor, "username")));

        pthread_mutex_unlock(&games_mutex);
    }
    else {
        cmn::sendData(connection_socket_descriptor, "error tag " + user_type + '\n');
        shutdown(connection_socket_descriptor, SHUT_RDWR);
    }

    pthread_mutex_lock(&connectionHandling_mutex);
    for (std::list<std::unique_ptr<pthread_t>>::iterator it = connectionHandlingThreads.begin(); it != connectionHandlingThreads.end(); ++it) {
        if (it->get() == thread) {
            connectionHandlingThreads.erase(it);
            break;
        }
    }
    pthread_mutex_unlock(&connectionHandling_mutex);
}

void Server::removeGame(Game* game) {
    pthread_mutex_lock(&games_mutex);

    while(game->getPlayerCount() > 0) {
        struct timespec ts;
        
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;

        pthread_cond_timedwait(&game->getRemoveGameCond(), &games_mutex, &ts);
    }

    std::vector<Game*>::iterator it = find(games.begin(), games.end(), game);

    if (it != games.end()) {
        delete *it;
        games.erase(it);
    }

    pthread_mutex_unlock(&games_mutex);
}

void Server::stop() {
    shutdown(server_socket_descriptor, SHUT_RDWR);
    pthread_mutex_lock(&games_mutex);

    for (std::vector<Game*>::iterator it = games.begin(); it != games.end() ;++it) {
        delete *it;
    }

    pthread_mutex_unlock(&games_mutex);
}