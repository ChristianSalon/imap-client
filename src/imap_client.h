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

class IMAPClient {
 public:
  enum class FetchOptions { ALL, HEADERS };

 protected:
  std::unique_ptr<Connection> connection;

  bool isLoggedIn{false};
  unsigned int tag{0};

  std::string mailbox{"inbox"};
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
