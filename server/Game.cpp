#include "Game.hpp"

unsigned int Game::game_id_seq = 0;

Game::Game(Server* serv, Connection* host) : host(host), serv(serv) { 
            setGameId();
            game_state = GameState::acceptingQuestions;
            host->setGame(this);
            this->host->start();
            players_mutex = PTHREAD_MUTEX_INITIALIZER;
            players_memory_release_cond = PTHREAD_COND_INITIALIZER;
            }


Game::~Game() {
    if (host != nullptr) host->terminateConnection();
}

pthread_cond_t& Game::getRemoveGameCond() {
    return players_memory_release_cond;
}

unsigned int Game::getPlayerCount() {
    return players.size();
}

unsigned int Game::getGameId()
{
    return game_id;
}

unsigned int Game::getGameId() const
{
    return game_id;
}

void Game::setGameId()
{
    this->game_id = Game::game_id_seq;
    ++Game::game_id_seq;
}

GameState Game::getGameState() {
    return game_state;
}

int Game::addPlayer(Connection* player) {
    if (game_state != GameState::roomOpen) return GameErrorCodes::gameRoomClosed; // dodanie gracza jedynie gdy pokój jest otwarty

    pthread_mutex_lock(&players_mutex);

    players.push_back(std::pair<Connection*, unsigned int>(player, 0));
    players.back().first->setGame(this);
    players.back().first->start();

    cmn::sendData(host->getConnectionSocketDescriptor(), "playerconn " + player->getUsername() + '\n');

    pthread_mutex_unlock(&players_mutex);

    return 0;
}

class CompPlayersGame {
    private:
        int connection_socket_descriptor;
    public:
        CompPlayersGame(int csd) : connection_socket_descriptor(csd) { }
        bool operator()(const std::pair<Connection*, unsigned int>& player) {
            return player.first->getConnectionSocketDescriptor() == connection_socket_descriptor;
        }
};

void Game::removePlayer(Connection* player) {
    pthread_mutex_lock(&players_mutex);

    std::vector<std::pair<Connection*, unsigned int>>::iterator it = std::find_if(players.begin(), players.end(), CompPlayersGame(player->getConnectionSocketDescriptor()));
    if (it != players.end()) {
        if (host != nullptr) cmn::sendData(host->getConnectionSocketDescriptor(), "playerdisconn " + player->getUsername() + '\n');
        delete it->first;
        players.erase(it);
    }
    
    if (players.size() == 0) {
        pthread_cond_signal(&players_memory_release_cond);
    }

    pthread_mutex_unlock(&players_mutex);
}

bool Game::operator==(const unsigned int& gid) const {
    return this->game_id == gid;
}

void Game::endGame(std::string message) {
   
    pthread_mutex_lock(&players_mutex);
    
    for (std::vector<std::pair<Connection*, unsigned int>>::iterator it = players.begin(); it != players.end(); ++it){
        cmn::sendData(it->first->getConnectionSocketDescriptor(), message);
        it->first->terminateConnection();
    }

    delete host;
    host = nullptr;
    pthread_mutex_unlock(&players_mutex);
    
    
    serv->removeGame(this);
}

void Game::sendQuestions() {
    pthread_mutex_lock(&players_mutex);
    std::string temp = "";
    if (!question_number){
            temp = "gs " + std::to_string(questions.size()) + '\n';
    }
    temp += "q " + questions[question_number].getQuestion() + '\n';
    for (unsigned int i = 0; i < questions[question_number].getAnswerVector().size(); ++i) {
        temp += "a " + questions[question_number].getAnswerVector()[i] + '\n';
    }
    
    for (std::vector<std::pair<Connection*, unsigned int>>::iterator it = players.begin(); it != players.end(); ++it) {
        cmn::sendData(it->first->getConnectionSocketDescriptor(), temp);
    }
    
    timer = std::chrono::high_resolution_clock::now();
    cmn::sendData(host->getConnectionSocketDescriptor(), "sent " + std::to_string(players.size()) + '\n'); // potwierdzenie wysłania pytań dla hosta
    
    ++question_number;
    pthread_mutex_unlock(&players_mutex);
}

int Game::nextState() {
    if (game_state == GameState::acceptingQuestions && questions.size() > 0) {
        game_state = GameState::roomOpen;
    }
    else if (game_state == GameState::roomOpen) {
        game_state = GameState::acceptingAnswers;
        sendQuestions();
    }
    else if (game_state == GameState::acceptingAnswers && question_number <= questions.size()-1) {
        sendQuestions();
    }
    else if (game_state == GameState::acceptingAnswers && question_number > questions.size()-1) {
        sendResults();
        game_state = GameState::gameEnd;
        host->terminateConnection();
    }
    return 0;
}

void Game::addQuestion(std::string s) {
    if(questions.size() && (questions.back().getAnswersSize() < 4 || questions.back().getCorrectAnswer() < 0 || questions.back().getCorrectAnswer() > 3)) {
        cmn::sendData(host->getConnectionSocketDescriptor(), "error q " + s + '\n');
        return;
    }
    questions.push_back(Question(s));
}

void Game::addAnswer(std::string s) {
    if(!questions.back().addAnswer(s)) {
        cmn::sendData(host->getConnectionSocketDescriptor(), "error a " + s + '\n');
    }
}

void Game::setCorrectAnswerIndex(unsigned int idx) {
    if(!questions.back().setCorrectAnswer(idx)) {
        cmn::sendData(host->getConnectionSocketDescriptor(), "error ca\n");
    }
}

const unsigned int Game::getNextQuestionIndex() {
    return question_number;
}

template<class A, class B> class PairExtractor {
    private:
        std::pair<A, B> val;
        int index;
    public:
        PairExtractor(std::pair<A, B> val, int index = 0) : val(val), index(index) { }
        bool operator()(const std::pair<A, B> elem) {
            if (index == 0) {
                return elem.first == val.first;
            }
            else {
                return elem.second == val.second;
            }
        }
};

void Game::checkAnswer(Connection* conn, unsigned int answer) {
    pthread_mutex_lock(&players_mutex);
    
    std::vector<std::pair<Connection*, unsigned int>>::iterator it = std::find_if(players.begin(), players.end(), PairExtractor<Connection*, unsigned int>(std::pair<Connection*, unsigned int>(conn, 0)));
    if (it != players.end()) {
        it->second += questions[question_number - 1].checkAnswer(answer);
    }

    pthread_mutex_unlock(&players_mutex);
}

bool Game::checkTimeUp() {
    return timer + std::chrono::seconds(15) < std::chrono::high_resolution_clock::now() ? true : false;
}

void Game::sendResults() {
    pthread_mutex_lock(&players_mutex);
    
    cmn::sendData(host->getConnectionSocketDescriptor(), "ranking\n");

    for (std::vector<std::pair<Connection*, unsigned int>>::iterator it = players.begin(); it != players.end(); ++it) {
        cmn::sendData(it->first->getConnectionSocketDescriptor(), ("points " + std::to_string(it->second) + '\n') );
        cmn::sendData(host->getConnectionSocketDescriptor(), (it->first->getUsername() + " " + std::to_string(it->second) + '\n') );
    }

    cmn::sendData(host->getConnectionSocketDescriptor(), "gniknar\n");
    
    pthread_mutex_unlock(&players_mutex);
}

bool Game::checkAddAnswers() {
    if (questions.size() && questions.back().getAnswerVector().size() < 4){
        return true;
    }
    return false;
}

bool Game::checkAddCorrectAnswer() {
    if (questions.size() && questions.back().getAnswerVector().size() == 4) {
        return true;
    }
    return false;
}