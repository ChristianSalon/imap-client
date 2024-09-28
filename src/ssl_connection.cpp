/**
 * @file ssl_connection.h
 * @author Christian Saloň
 */

#include "ssl_connection.h"

SSLConnection::SSLConnection(std::string hostname,
                             uint16_t port,
                             std::string certificateFile,
                             std::string certificatesFolderPath) {
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  OpenSSL_add_all_algorithms();

  this->ctx = SSL_CTX_new(SSLv23_client_method());
  if (this->ctx == nullptr) {
    throw std::runtime_error("Could not create ssl context.");
  }
  if (!SSL_CTX_load_verify_locations(this->ctx, certificateFile.empty() ? nullptr : certificateFile.c_str(),
                                     certificatesFolderPath.c_str())) {
    throw std::runtime_error("Could not verify certificates folder.");
  }

  this->bio = BIO_new_ssl_connect(this->ctx);
  if (this->bio == nullptr) {
    throw std::runtime_error("Could not create ssl connection to server.");
  }

  BIO_get_ssl(this->bio, &this->ssl);
  SSL_set_mode(this->ssl, SSL_MODE_AUTO_RETRY);

  std::string connectionString = hostname + ":" + std::to_string(static_cast<int>(port));
  BIO_set_conn_hostname(this->bio, connectionString.c_str());

  if (BIO_do_connect(this->bio) <= 0) {
    throw std::runtime_error("Could not connect to server.");
  }

  /*if(SSL_get_verify_result(this->ssl) != X509_V_OK) {
      throw std::runtime_error("Certificate sent from the server is not
  valid.");
  }*/
}

SSLConnection::~SSLConnection() {
  if (this->bio)
    BIO_free_all(this->bio);

  if (this->ssl)
    SSL_free(this->ssl);

  if (this->ctx)
    SSL_CTX_free(this->ctx);
}

std::string SSLConnection::sendCommand(unsigned int tag, std::string command) {
  static int MAX_RETRIES = 8;

  // Send command to server
  if (BIO_write(this->bio, command.data(), command.length()) <= 0) {
    if (!BIO_should_retry(this->bio)) {
      throw std::runtime_error("Could not send command to server.");
    }

    int retryCount = 0;
    while (retryCount < MAX_RETRIES) {
      if (BIO_write(this->bio, command.data(), command.length()) > 0) {
        break;
      }

      if (!BIO_should_retry(this->bio)) {
        throw std::runtime_error("Could not send command to server.");
      }
      retryCount++;
    }

    throw std::runtime_error("Could not send command to server.");
  }

  // Get response from server
  std::string response = this->receive();
  while (!this->isResponseFull(response, tag)) {
    response.append(this->receive());
  }

  return response;
}

std::string SSLConnection::receive() {
  static int MAX_RETRIES = 8;

  std::string response;
  std::string buffer;

  while (true) {
    buffer.resize(1500);

    int bytes = BIO_read(this->bio, &buffer[0], buffer.size());
    if (bytes == 0) {
      throw std::runtime_error("Server closed connection.");
    } else if (bytes < 0) {
      if (!BIO_should_retry(this->bio)) {
        throw std::runtime_error("Could not receive data from server.");
      }

      int retryCount = 0;
      while (retryCount < MAX_RETRIES) {
        int bytes = BIO_read(this->bio, &buffer[0], buffer.size());
        if (bytes > 0) {
          break;
        } else if (bytes == 0) {
          throw std::runtime_error("Server closed connection.");
        }

        if (!BIO_should_retry(this->bio)) {
          throw std::runtime_error("Could not receive data from server.");
        }
        retryCount++;
      }
    }

    response.append(buffer.substr(0, bytes));
    if (response.ends_with("\r\n")) {
      break;
    }
  }

  return response;
}
