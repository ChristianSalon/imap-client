/**
 * @file imap_client.h
 * @author Christian Saloň
 */

#ifndef IMAP_CLIENT_H
#define IMAP_CLIENT_H

#include <algorithm>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "connection.h"
#include "ssl_connection.h"
#include "tcp_connection.h"

/**
 * @brief Represents a imap client
 */
class IMAPClient {
 public:
  /// @brief Represent whether to fetch only headers or the full contents of an email
  enum class FetchOptions { ALL, HEADERS };

 protected:
  /// @brief Connection to an imap server
  std::unique_ptr<Connection> connection;

  /// @brief Represents if the user is logged in
  bool isLoggedIn{false};
  /// @brief Tag used in commands that are sent to the server
  unsigned int tag{0};

  /// @brief Selected mailbox
  std::string mailbox{"inbox"};
  /// @brief Represents if the selected mailbox is empty
  bool isMailboxEmpty{true};

 public:
  IMAPClient(std::string hostname, uint16_t port);
  IMAPClient(std::string hostname, uint16_t port, std::string certificateFile, std::string certificatesFolderPath);
  ~IMAPClient();

  void login(std::string username, std::string password);
  void logout();

  void select(std::string mailbox);
  std::unordered_map<std::string, std::string> fetch(FetchOptions options);
  std::unordered_map<std::string, std::string> fetchNew(FetchOptions options);

 protected:
  std::unordered_map<std::string, std::string> parseEmails(std::string fetchResponse);
  std::string getNewEmailUIDs();

  std::string toLowerCase(std::string& input);
};

#endif
