/**
 * @file tcp_connection.cpp
 * @author Christian Saloň
 */

#include "tcp_connection.h"

TCPConnection::TCPConnection(std::string hostname, uint16_t port) {
  struct addrinfo hints {};
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* addresses = nullptr;
  int addressCount = getaddrinfo(hostname.c_str(), nullptr, &hints, &addresses);
  if (addressCount != 0 || addresses == nullptr) {
    throw std::runtime_error("Could not get server address.");
  }

  sockaddr_in address = *reinterpret_cast<sockaddr_in*>(addresses->ai_addr);
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  freeaddrinfo(addresses);

  this->serverAddress = *reinterpret_cast<struct sockaddr*>(&address);

  this->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (this->clientSocket <= 0) {
    throw std::runtime_error("Could not create socket.");
  }

  if (connect(this->clientSocket, &this->serverAddress, sizeof(this->serverAddress)) != 0) {
    throw std::runtime_error("Could not connect to server by TCP.");
  }
}

TCPConnection::~TCPConnection() {
  shutdown(this->clientSocket, SHUT_RDWR);
  close(this->clientSocket);
}

std::string TCPConnection::sendCommand(unsigned int tag, std::string command) {
  // Send command to server
  int bytes = send(this->clientSocket, command.data(), command.size(), 0);
  if (bytes < 0) {
    throw std::runtime_error("Could not send command to server.");
  }

  std::cout << command;

  // Get response from server
  std::string response = this->receive();
  while (!this->isResponseFull(response, tag)) {
    response.append(this->receive());
  }

  std::cout << response;

  return response;
}

std::string TCPConnection::receive() {
  std::string response;
  std::string buffer;
  socklen_t socketSize = sizeof(this->serverAddress);

  while (true) {
    buffer.resize(1500);
    long bytes = recv(this->clientSocket, &buffer[0], buffer.size(), 0);
    if (bytes <= 0) {
      throw std::runtime_error("Could not receive data from server.");
    }

    response.append(buffer.substr(0, bytes));
    if (response.ends_with("\r\n")) {
      break;
    }
  }

  return response;
}
