#include "Connection.hpp"
#include "Game.hpp"
#include <sstream>

Connection::Connection(const Connection& conn) {
        this->type = conn.type;
        this->connection_socket_descriptor = conn.connection_socket_descriptor;
        this->username = conn.username;
        this->thread = conn.thread;
        this->gameptr = conn.gameptr;
}

Connection::~Connection() {
    gameptr = nullptr;
}


void Connection::setGame(Game* gameptr) {
    this->gameptr = gameptr;
}

int Connection::getConnectionSocketDescriptor() {
    return connection_socket_descriptor;
}

ConnectionTypes Connection::getType() {
    return type;
}

std::string Connection::getUsername() {
    return username;
}

void Connection::start() {
    int create_result = 0;
    create_result = pthread_create(&thread, NULL, threadRoutine, this);
}

void* Connection::threadRoutine(void* arg) {
    pthread_detach(pthread_self());

    Connection* conn = static_cast<Connection*>(arg);
    if (conn->getType() == ConnectionTypes::player) {
        conn->playerBehaviour();
    }
    else if (conn->getType() == ConnectionTypes::host) {
        conn->hostBehaviour();
    }

    pthread_exit(NULL);
}

void Connection::hostBehaviour() {
    //std::string temp = "You are the host! Your game id is " + std::to_string(gameptr->getGameId()) + "\nYour name is " + username + '\n';
    std::string temp = "gid " + std::to_string(gameptr->getGameId()) + '\n';
    cmn::sendData(connection_socket_descriptor, temp);

    //unsigned int data_size;
    std::string data, tag, value;
    std::stringstream ss;

    while(!disconnect) {
        //data_size = cmn::getDataSize(connection_socket_descriptor);
        data = cmn::readData(connection_socket_descriptor);

        ss = std::stringstream(data);
        ss >> tag;
        if (!ss.eof()) {
            std::getline(ss, value);
            value = value.substr(1, value.length() - 1); // removing leading whitespace from getline
        }
        if (tag == "disconnect") {
            terminateConnection();
        }
        else if (tag == "q" && gameptr->getGameState() == GameState::acceptingQuestions) {
            gameptr->addQuestion(value);
        }
        else if (tag == "a" && gameptr->getGameState() == GameState::acceptingQuestions && gameptr->checkAddAnswers()) {
            gameptr->addAnswer(value);
        }
        else if (tag == "ca" && gameptr->getGameState() == GameState::acceptingQuestions && gameptr->checkAddCorrectAnswer()) {
            try {
                gameptr->setCorrectAnswerIndex(std::stoul(value) - 1);
            } catch (std::invalid_argument& e){
                cmn::sendData(connection_socket_descriptor, "error " + value + " nan\n");
            }
        }
        else if (tag == "open" && gameptr->getGameState() == GameState::acceptingQuestions) {
            gameptr->nextState();
        }
        else if (tag == "begin" && gameptr->getGameState() == GameState::roomOpen) {
            gameptr->nextState();
        }
        else if (tag == "next" && gameptr->getGameState() == GameState::acceptingAnswers && gameptr->checkTimeUp()) {
            gameptr->nextState();
        }
        else {
            cmn::sendData(connection_socket_descriptor, "error " + tag + "\n");
        }
        tag = "";
    }

    gameptr->endGame();
}

void Connection::playerBehaviour() {
    //std::string temp = "You've successfully connected! Your game id is " + std::to_string(gameptr->getGameId()) + "\nYour name is " + username + '\n';
    std::string temp = "gc connected\n";
    cmn::sendData(connection_socket_descriptor, temp);

    //unsigned int data_size;
    std::string data, tag, value;
    std::stringstream ss;

    unsigned int next_question_index = 1;

    while(!disconnect) {
        //data_size = cmn::getDataSize(connection_socket_descriptor);
        data = cmn::readData(connection_socket_descriptor);
        if (data.length() > 0) {
            ss = std::stringstream(data);
            ss >> tag;
            if (!ss.eof()) {
                std::getline(ss, value);
                value = value.substr(1, value.length() - 1); // removing leading whitespace from getline
            }

            if (tag == "disconnect") {
                terminateConnection();
            }
            if (tag == "ca" && gameptr->getGameState() == GameState::acceptingAnswers) {
                if (next_question_index != gameptr->getNextQuestionIndex()) {
                    //cmn::sendData(connection_socket_descriptor, "error a\n");
                }
                else if (gameptr->checkTimeUp()) {
                    //cmn::sendData(connection_socket_descriptor, "timeup\n");
                }
                else {
                    try {
                        //printf("%s: %s,%lu\n", username.c_str(), value.c_str(), std::stoul(value));
                        gameptr->checkAnswer(this, std::stoul(value) - 1); // dla intuicji odpowiedzi od 1 do 4
                        ++next_question_index;
                    } catch (std::invalid_argument& e){
                        //cmn::sendData(connection_socket_descriptor, "error " + value + " nan\n");
                    }
                }
            }
            else {
                cmn::sendData(connection_socket_descriptor, "error " + tag + "\n");
            }
        }
    }

    gameptr->removePlayer(this);
}

void Connection::terminateConnection() {
    disconnect = true;
    if (type == ConnectionTypes::player) {
        cmn::sendData(connection_socket_descriptor, "gc closed\n");
        shutdown(connection_socket_descriptor, SHUT_RDWR);
        //close(connection_socket_descriptor);
    }
    else if (type == ConnectionTypes::host) {
        cmn::sendData(connection_socket_descriptor, "gc gameend\n");
        shutdown(connection_socket_descriptor, SHUT_RDWR);
        //close(connection_socket_descriptor);
    }
}