#include "communication.hpp"
#include "constants.hpp"

#include <sys/signal.h>
#include <errno.h>


unsigned int cmn::getDataSize(int connection_socket_descriptor) {
    unsigned int result;
    unsigned int left = sizeof(result); // pozostało do odczytu

    unsigned char* data = (unsigned char*)&result; // wskaźnik na odpowiedni typ danych

    int cread; // odczytana liczba znaków

    //if (DEBUG) left *= 2; // result za małe

    do {
        cread = read(connection_socket_descriptor, data, left);
        if (cread < 0) {
            printf("Błąd odczytu rozmiaru danych, kod błędu: %d\n", cread);
            exit(-1);
        }
        else {
            left -= cread;
            data += cread; // uwaga: tutaj jest przsunięcie wskaźnika
        }
    } while (left);

    if (DEBUG) {
        for (unsigned int i = 0; i < sizeof(data); ++i) {
            *(data - i) -= 'A';
        }
    }
    return result;
}

// std::string cmn::readData(int connection_socket_descriptor, unsigned int data_size) {
//     std::string data(data_size, 0);
//     unsigned int left = data_size;
//     if (DEBUG) {
//         left += 1;
//         data_size += 1;
//     }

//     int cread;

//     do {
//         cread = read(connection_socket_descriptor, &data[data_size - left], left);
//         printf("Odczytano %d bajtów. Pozostalo %u bajtów\n", cread, left - cread);
//         printf("%s\n", data.c_str());
//         if (cread < 0) {
//             printf("Błąd odczytu danych, kod błędu: %d\n", cread);
//             exit(-1);
//         }
//         else {
//             left -= cread;
//         }
//     } while (left);

//     //if (DEBUG) return data.substr(0, data.length());
//     return data;
// }

std::string cmn::readData(int connection_socket_descriptor) {
    std::string data;
    char buff;
    int cread;

    

    do {
        cread = read(connection_socket_descriptor, &buff, 1);
        // if (cread < 0) {
        //     printf("Błąd odczytu danych, kod błędu: %d\n", cread);
        //     exit(-1);
        // }
        /*else*/ if (cread <= 0) {
            // Prawdopodobnie zerwanie połączenia
            data = "disconnect\n";
            break;
        }
        else {
            data += buff;
        }
    } while (buff != '\n');

    return data.substr(0, data.length() - 1 >= 0 ? data.length() - 1 : 0);
}

void cmn::sendData(int connection_socket_descriptor, std::string data) {
    sigset_t sig_block, sig_restore, sig_pending;

    sigemptyset(&sig_block);
    sigaddset(&sig_block, SIGPIPE);

    if (pthread_sigmask(SIG_BLOCK, &sig_block, &sig_restore) != 0) {
        return;
    }

    int sigpipe_pending = -1;
    if (sigpending(&sig_pending) != -1) {
        sigpipe_pending = sigismember(&sig_pending, SIGPIPE);
    }

    if (sigpipe_pending == -1) {
        pthread_sigmask(SIG_SETMASK, &sig_restore, NULL);
        return;
    }

    unsigned int dataLeft = data.length();
    int cwrite;

    do {
        cwrite = write(connection_socket_descriptor, &data[data.length() - dataLeft], dataLeft);
        if (cwrite <= 0) {
            //printf("Błąd wysyłania danych, kod błędu: %d\n", cwrite);
            //exit(-1);
            break;
        }
        else {
            dataLeft -= cwrite;
        }
    } while(dataLeft);

    if (errno == EPIPE && sigpipe_pending == 0) {
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = 0;

        int sig;
        while ((sig = sigtimedwait(&sig_block, 0, &ts)) == -1) {
            if (errno != EINTR)
                break;
        }
    }

    pthread_sigmask(SIG_SETMASK, &sig_restore, NULL);
}
