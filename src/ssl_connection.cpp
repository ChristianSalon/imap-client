/**
 * @file ssl_connection.h
 * @author Christian Saloň
 */

#include "ssl_connection.h"

/**
 * @brief Construct a new SSLConnection object
 *
 * @param hostname Server hostname
 * @param port Server port
 * @param certificateFile Path to a certificate file used for validating ssl/tls certificate
 * @param certificatesFolderPath Path to a folder which is used for validating ssl/tls certificates
 */
SSLConnection::SSLConnection(std::string hostname,
                             uint16_t port,
                             std::string certificateFile,
                             std::string certificatesFolderPath) {
  // Initialize openssl library
  SSL_load_error_strings();
  ERR_load_BIO_strings();
  OpenSSL_add_all_algorithms();

  // Setup ssl context
  this->ctx = SSL_CTX_new(SSLv23_client_method());
  if (this->ctx == nullptr) {
    throw std::runtime_error("Could not create ssl context.");
  }

  // Laod trust certificate store used for validating certificates
  if (!SSL_CTX_load_verify_locations(this->ctx, certificateFile.empty() ? nullptr : certificateFile.c_str(),
                                     certificatesFolderPath.c_str())) {
    throw std::runtime_error("Could not verify certificates folder.");
  }

  // Setup bio object
  this->bio = BIO_new_ssl_connect(this->ctx);
  if (this->bio == nullptr) {
    throw std::runtime_error("Could not create ssl connection to server.");
  }
  BIO_get_ssl(this->bio, &this->ssl);
  SSL_set_mode(this->ssl, SSL_MODE_AUTO_RETRY);

  // Open secure connection
  std::string connectionString = hostname + ":" + std::to_string(static_cast<int>(port));
  BIO_set_conn_hostname(this->bio, connectionString.c_str());
  if (BIO_do_connect(this->bio) <= 0) {
    throw std::runtime_error("Could not connect to server.");
  }

  // Check if the certificate sent from the server is valid
  if (SSL_get_verify_result(this->ssl) != X509_V_OK) {
    throw std::runtime_error("Certificate sent from the server is not valid.");
  }
}

/**
 * @brief Destroy the SSLConnection object
 */
SSLConnection::~SSLConnection() {
  if (this->bio) {
    BIO_free_all(this->bio);
  }

  if (this->ssl) {
    SSL_free(this->ssl);
  }

  if (this->ctx) {
    SSL_CTX_free(this->ctx);
  }
}

/**
 * @brief Send an imap command to the server
 *
 * @param tag Command tag
 * @param command Command to send
 * @return std::string Response from the server
 */
std::string SSLConnection::sendCommand(unsigned int tag, std::string command) {
  static int MAX_RETRIES = 8;

  // Send command to server
  if (BIO_write(this->bio, command.data(), command.length()) <= 0) {
    // Sending command failed and should not be sent again
    if (!BIO_should_retry(this->bio)) {
      throw std::runtime_error("Could not send command to server.");
    }

    // Sending command failed but should be sent again
    int retryCount = 0;
    while (retryCount < MAX_RETRIES) {
      // Send command to server
      if (BIO_write(this->bio, command.data(), command.length()) > 0) {
        break;
      }

      // Sending command failed and should not be sent again
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
std::string SSLConnection::receive() {
  static int MAX_RETRIES = 8;

  std::string response;
  std::string buffer;

  // Receive data until it ends in "\r\n"
  // "\r\n" possibly indicates end of a response
  while (true) {
    buffer.resize(1500);

    int bytes = BIO_read(this->bio, &buffer[0], buffer.size());
    if (bytes == 0) {
      throw std::runtime_error("Server closed connection.");
    } else if (bytes < 0) {
      // Receiving data failed and should not try again
      if (!BIO_should_retry(this->bio)) {
        throw std::runtime_error("Could not receive data from server.");
      }

      // Receiving data failed but should try again
      int retryCount = 0;
      while (retryCount < MAX_RETRIES) {
        int bytes = BIO_read(this->bio, &buffer[0], buffer.size());
        if (bytes > 0) {
          // Data successfuly received
          break;
        } else if (bytes == 0) {
          throw std::runtime_error("Server closed connection.");
        }

        if (!BIO_should_retry(this->bio)) {
          // Receiving data failed and should not try again
          throw std::runtime_error("Could not receive data from server.");
        }

        retryCount++;
      }
    }

    // Add received data to buffer
    response.append(buffer.substr(0, bytes));
    if (response.ends_with("\r\n")) {
      break;
    }
  }

  return response;
}
