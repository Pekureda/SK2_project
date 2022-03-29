#ifndef QUESTION_HPP
#define QUESTION_HPP

#include <string>
#include <algorithm>
#include <vector>

class Question {
    private:
        std::string question;
        std::vector<std::string> possible_answers;
        unsigned int correct_answer_index;

    public:
        Question(std::string question) : question(question) { }


        bool checkAnswer(unsigned int answer) {
            return answer == correct_answer_index ? true : false;
        }

        bool addAnswer(std::string a) {
            if (possible_answers.size() < 4) {
                possible_answers.push_back(a);
                return true;
            }
            return false;
        }

        bool setCorrectAnswer(unsigned int idx) {
            if (possible_answers.size() < 4 || idx > 3)
                return false;
            correct_answer_index = idx;
            return true;
        }

        unsigned int getAnswersSize() {
            return possible_answers.size();
        }

        std::string getQuestion() {
            return question;
        }
        std::vector<std::string> getAnswerVector() {
            return possible_answers;
        }

        unsigned int getCorrectAnswer() {
            return correct_answer_index;
        }
};

#endif