#ifndef COMMUNICATION_HPP
#define COMMUNICATION_HPP

#include <string>
#include <unistd.h>

namespace cmn {
    unsigned int getDataSize(int connection_socket_descriptor);
    // std::string readData(int connection_socket_descriptor, unsigned int data_size);
    std::string readData(int connection_socket_descriptor);
    void sendData(int connection_socket_descriptor, std::string data);

};

#endif