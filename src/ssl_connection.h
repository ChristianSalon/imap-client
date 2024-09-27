/**
 * @file ssl_connection.h
 * @author Christian Saloň
 */

#ifndef SSL_CONNECTION_H
#define SSL_CONNECTION_H

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>

#include "openssl/bio.h"
#include "openssl/err.h"
#include "openssl/ssl.h"

#include "connection.h"

class SSLConnection : public Connection {
 protected:
  BIO* bio;
  SSL_CTX* ctx;
  SSL* ssl;

 public:
  SSLConnection(std::string hostname, uint16_t port, std::string certificateFile, std::string certificatesFolderPath);
  ~SSLConnection();

  std::string sendCommand(unsigned int tag, std::string command);
  std::string receive();
};

#endif
