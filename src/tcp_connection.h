/**
 * @file tcp_connection.h
 * @author Christian Saloň
 */

#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "connection.h"

class TCPConnection : public Connection {
 protected:
  sockaddr serverAddress;
  int clientSocket;

 public:
  TCPConnection(std::string hostname, uint16_t port);
  ~TCPConnection();

  std::string sendCommand(unsigned int tag, std::string command);
  std::string receive();
};

#endif
