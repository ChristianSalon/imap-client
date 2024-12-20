/**
 * IMAP client
 *
 * @file tcp_connection.cpp
 * @author Christian Saloň <xsalon02>
 */

#include "tcp_connection.h"

/**
 * @brief Construct a new TCPConnection object
 *
 * @param hostname Server hostname
 * @param port Server port
 */
TCPConnection::TCPConnection(std::string hostname, uint16_t port) {
  // Get ip address of server
  struct addrinfo hints {};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo *addresses = nullptr;
  int addressCount = getaddrinfo(hostname.c_str(), nullptr, &hints, &addresses);
  if (addressCount != 0 || addresses == nullptr) {
    throw std::runtime_error("Could not get server address.");
  }

  sockaddr_in ipv4Address;
  sockaddr_in6 ipv6Address;
  int addressFamily = -1;

  // Loop retrieved addresses
  for (addrinfo *address = addresses; address != nullptr; address = address->ai_next) {
    if (address->ai_family == AF_INET) {
      ipv4Address = *reinterpret_cast<struct sockaddr_in *>(address->ai_addr);
      addressFamily = AF_INET;
      break;
    } else if (address->ai_family == AF_INET6) {
      ipv6Address = *reinterpret_cast<struct sockaddr_in6 *>(address->ai_addr);
      addressFamily = AF_INET6;
      break;
    }
  }

  freeaddrinfo(addresses);

  if (addressFamily == AF_INET) {
    ipv4Address.sin_port = htons(port);
    this->serverAddress = *reinterpret_cast<struct sockaddr *>(&ipv4Address);

    // Create TCP socket
    this->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  } else if (addressFamily == AF_INET6) {
    ipv6Address.sin6_port = htons(port);
    this->serverAddress = *reinterpret_cast<struct sockaddr *>(&ipv6Address);

    // Create TCP socket
    this->clientSocket = socket(AF_INET6, SOCK_STREAM, 0);
  } else {
    throw std::runtime_error("Could not find ipv4 or ipv6 address of server.");
  }

  if (this->clientSocket <= 0) {
    throw std::runtime_error("Could not create socket.");
  }

  // Connect to server
  if (connect(this->clientSocket, &this->serverAddress, sizeof(this->serverAddress)) != 0) {
    throw std::runtime_error("Could not connect to server by TCP.");
  }
}
/**
 * @brief Construct a new TCPConnection object
 *
 * @param fd Socket file descriptor
 */
TCPConnection::TCPConnection(int fd) : clientSocket{fd} {}

/**
 * @brief Closes the connection to server
 */
void TCPConnection::closeConnection() {
  shutdown(this->clientSocket, SHUT_RDWR);
  close(this->clientSocket);
}

/**
 * @brief Send an imap command to the server
 *
 * @param tag Command tag
 * @param command Command to send
 * @return std::string Response from the server
 */
std::string TCPConnection::sendCommand(unsigned int tag, std::string command) {
  // Send command to server
  int bytes = send(this->clientSocket, command.data(), command.size(), 0);
  if (bytes < 0) {
    throw std::runtime_error("Could not send command to server.");
  }

  // Get response from server
  std::string response = this->receive();
  while (!this->isResponseFull(response, tag)) {
    // Receive more data because the response is not complete
    response.append(this->receive());
  }

  return response;
}

/**
 * @brief Receive data from the server
 *
 * @return std::string Received data
 */
std::string TCPConnection::receive() {
  std::string response;
  std::string buffer;

  // Receive data until it ends in "\r\n"
  // "\r\n" possibly indicates end of a response
  while (true) {
    buffer.resize(1500);

    // Receive data from socket
    long bytes = recv(this->clientSocket, &buffer[0], buffer.size(), 0);
    if (bytes <= 0) {
      throw std::runtime_error("Could not receive data from server.");
    }

    // Add received data to buffer
    response.append(buffer.substr(0, bytes));
    if (response.ends_with("\r\n")) {
      break;
    }
  }

  return response;
}

int TCPConnection::getFd() {
  return this->clientSocket;
}
