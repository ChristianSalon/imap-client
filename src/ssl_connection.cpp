/**
 * IMAP client
 *
 * @file ssl_connection.h
 * @author Christian Saloň <xsalon02>
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
                             std::string certificatesFolderPath)
    : TCPConnection{hostname, port} {
  // Initialize openssl library
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  // Setup ssl context
  this->ctx = SSL_CTX_new(SSLv23_client_method());
  if (this->ctx == nullptr) {
    throw std::runtime_error("Could not create ssl context.");
  }

  // Laod trust certificate store used for validating certificates
  if (!SSL_CTX_load_verify_locations(this->ctx, certificateFile.empty() ? nullptr : certificateFile.c_str(),
                                     certificatesFolderPath.empty()
                                         ? SSLConnection::DEFAULT_CERTIFICATES_FOLDER_PATH.c_str()
                                         : certificatesFolderPath.c_str())) {
    throw std::runtime_error("Could not verify certificates folder.");
  }

  this->ssl = SSL_new(this->ctx);
  SSL_set_mode(this->ssl, SSL_MODE_AUTO_RETRY);

  // Set file descriptor used in unsecure connection
  if (SSL_set_fd(this->ssl, this->clientSocket) <= 0) {
    throw std::runtime_error("Could not create ssl connection to server from existing socket.");
  }

  if (SSL_connect(this->ssl) <= 0) {
    throw std::runtime_error("Could not connect perform SSL handshake.");
  }

  // Check if the certificate sent from the server is valid
  if (SSL_get_verify_result(this->ssl) != X509_V_OK) {
    throw std::runtime_error("Certificate sent from the server is not valid.");
  }
}

/**
 * @brief Construct a new SSLConnection object
 *
 * @param fd Socket file descriptror
 * @param certificateFile Path to a certificate file used for validating ssl/tls certificate
 * @param certificatesFolderPath Path to a folder which is used for validating ssl/tls certificates
 */
SSLConnection::SSLConnection(int fd, std::string certificateFile, std::string certificatesFolderPath)
    : TCPConnection{fd} {
  // Initialize openssl library
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  // Setup ssl context
  this->ctx = SSL_CTX_new(SSLv23_client_method());
  if (this->ctx == nullptr) {
    throw std::runtime_error("Could not create ssl context.");
  }

  // Laod trust certificate store used for validating certificates
  if (!SSL_CTX_load_verify_locations(this->ctx, certificateFile.empty() ? nullptr : certificateFile.c_str(),
                                     certificatesFolderPath.empty()
                                         ? SSLConnection::DEFAULT_CERTIFICATES_FOLDER_PATH.c_str()
                                         : certificatesFolderPath.c_str())) {
    throw std::runtime_error("Could not verify certificates folder.");
  }

  this->ssl = SSL_new(this->ctx);
  SSL_set_mode(this->ssl, SSL_MODE_AUTO_RETRY);

  // Set file descriptor used in unsecure connection
  if (SSL_set_fd(this->ssl, fd) <= 0) {
    throw std::runtime_error("Could not create ssl connection to server from existing socket.");
  }

  if (SSL_connect(this->ssl) <= 0) {
    throw std::runtime_error("Could not connect perform SSL handshake.");
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
  if (this->ssl) {
    SSL_free(this->ssl);
  }

  if (this->ctx) {
    SSL_CTX_free(this->ctx);
  }

  this->closeConnection();
}

/**
 * @brief Send an imap command to the server
 *
 * @param tag Command tag
 * @param command Command to send
 * @return std::string Response from the server
 */
std::string SSLConnection::sendCommand(unsigned int tag, std::string command) {
  // Send command to server
  int bytes = SSL_write(this->ssl, command.data(), command.size());
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
std::string SSLConnection::receive() {
  std::string response;
  std::string buffer;

  // Receive data until it ends in "\r\n"
  // "\r\n" possibly indicates end of a response
  while (true) {
    buffer.resize(1500);

    // Receive data from socket
    long bytes = SSL_read(this->ssl, &buffer[0], buffer.size());
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
