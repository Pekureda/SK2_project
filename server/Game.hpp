#ifndef GAME_HPP
#define GAME_HPP

#include <vector>
#include <chrono>

#include "constants.hpp"
#include "Connection.hpp"
#include "Question.hpp"
#include "Server.hpp"

class Server;

enum class GameState {
    acceptingQuestions,
    roomOpen,
    roomClosed,
    acceptingAnswers,
    gameEnd
};

enum GameErrorCodes {
    gameRoomClosed = INT32_MIN
};

class Game {
    private:
        static unsigned int game_id_seq;

        unsigned int game_id;
        Connection* host;
        Server* serv;
        std::vector<Question> questions;
        unsigned int question_number = 0;
        GameState game_state;

        std::chrono::system_clock::time_point timer;

        pthread_mutex_t players_mutex;
        pthread_cond_t players_memory_release_cond;
        std::vector<std::pair<Connection*, unsigned int>> players;
    public:
        Game(Server* serv, Connection* host);
        ~Game();
        bool operator==(const unsigned int& gid) const;

        pthread_cond_t& getRemoveGameCond();

        void setGameId();
        unsigned int getGameId();
        unsigned int getGameId() const;
        unsigned int getPlayerCount();
        int addPlayer(Connection* player);
        void removePlayer(Connection* player);
        
        void addQuestion(std::string s);
        void addAnswer(std::string s);
        void setCorrectAnswerIndex(unsigned int idx);
        void sendResults();
        void sendQuestions();
        const unsigned int getNextQuestionIndex();
        void checkAnswer(Connection* conn, unsigned int answer);
        bool checkTimeUp();

        bool checkAddAnswers();
        bool checkAddCorrectAnswer();

        void endGame(std::string message = "Host ended the game.\n");

        // Game logic is here
        int nextState();

        GameState getGameState();
};

#endif