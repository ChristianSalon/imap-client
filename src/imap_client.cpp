/**
 * @file imap_client.cpp
 * @author Christian Saloň
 */

#include "imap_client.h"

IMAPClient::IMAPClient(std::string hostname,
                       uint16_t port,
                       std::string certificateFile,
                       std::string certificatesFolderPath) {
  // Create a connection to server
  connection = std::make_unique<SSLConnection>(hostname, port, certificateFile, certificatesFolderPath);

  // Receive server greeting
  this->connection->receive();
}

IMAPClient::IMAPClient(std::string hostname, uint16_t port) {
  // Create a connection to server
  connection = std::make_unique<TCPConnection>(hostname, port);

  // Receive server greeting
  this->connection->receive();
}

IMAPClient::~IMAPClient() {}

void IMAPClient::login(std::string username, std::string password) {
  // Send LOGIN command to server
  std::string command = std::to_string(this->tag) + " login " + username + " " + password + "\r\n";
  std::string response = this->connection->sendCommand(this->tag, command);

  // Verify that login was successful
  if (response.find(std::to_string(this->tag) + " OK") == std::string::npos) {
    throw std::runtime_error("Invalid auth credentials.");
  }

  this->isLoggedIn = true;
  this->tag++;
}

void IMAPClient::logout() {
  // Send LOGOUT command to server
  std::string command = std::to_string(this->tag) + " logout\r\n";
  std::string response = this->connection->sendCommand(this->tag, command);

  // Verify that logout was successful
  if (response.find(std::to_string(this->tag) + " OK") == std::string::npos) {
    throw std::runtime_error("Could not logout.");
  }

  this->isLoggedIn = false;
  this->tag++;
}

void IMAPClient::select(std::string mailbox) {
  // Send SELECT command to server
  std::string command = std::to_string(this->tag) + " select " + mailbox + "\r\n";
  std::string response = this->connection->sendCommand(this->tag, command);

  // Verify that selecting mailbox was successful
  if (response.find(std::to_string(this->tag) + " OK") == std::string::npos) {
    throw std::runtime_error("Could not select mailbox.");
  }
  this->mailbox = mailbox;

  // Check if mailbox is empty
  unsigned long emailCountEnd = this->toLowerCase(response).find(" exists");
  unsigned long emailCountStart = response.substr(0, emailCountEnd).find_last_of(" ");
  if (emailCountStart == std::string::npos || emailCountEnd == std::string::npos) {
    throw std::runtime_error("Invalid select response format.");
  }
  int emailCount = std::stoi(response.substr(emailCountStart + 1, emailCountEnd - emailCountStart - 1));
  this->isMailboxEmpty = emailCount == 0;

  this->tag++;
}

std::unordered_map<std::string, std::string> IMAPClient::fetch(FetchOptions options) {
  if (!this->isLoggedIn) {
    throw std::runtime_error("User must be logged in before fetching emails.");
  }

  if (this->isMailboxEmpty) {
    throw std::runtime_error("Selected mailbox is empty.");
  }

  // Send FETCH command to server
  std::string command =
      std::to_string(this->tag) + " fetch 1:* rfc822" + (options == FetchOptions::ALL ? "" : ".header") + "\r\n";
  std::string response = this->connection->sendCommand(this->tag, command);

  // Verify that fetching emails was successful
  if (response.find(std::to_string(this->tag) + " OK") == std::string::npos) {
    throw std::runtime_error("Could not fetch emails.");
  }

  this->tag++;
  return this->parseEmails(response);
}

std::unordered_map<std::string, std::string> IMAPClient::fetchNew(FetchOptions options) {
  if (!this->isLoggedIn) {
    throw std::runtime_error("User must be logged in before fetching emails.");
  }

  if (this->isMailboxEmpty) {
    throw std::runtime_error("Selected mailbox is empty.");
  }

  // Get UIDs of new emails
  std::string uids = this->getNewEmailUIDs();
  if (uids.empty()) {
    return {};
  }

  // Send FETCH command to server
  std::string command = std::to_string(this->tag) + " fetch " + uids + " rfc822" +
                        (options == FetchOptions::ALL ? "" : ".header") + "\r\n";
  std::string response = this->connection->sendCommand(this->tag, command);

  // Verify that fetching emails was successful
  if (response.find(std::to_string(this->tag) + " OK") == std::string::npos) {
    throw std::runtime_error("Could not fetch emails.");
  }

  this->tag++;
  return this->parseEmails(response);
}

std::unordered_map<std::string, std::string> IMAPClient::parseEmails(std::string fetchResponse) {
  std::unordered_map<std::string, std::string> emails;
  unsigned long pointer = 0;
  unsigned long lastRowStartIndex = fetchResponse.substr(0, fetchResponse.length() - 2).find_last_of("\r\n");

  while (pointer < lastRowStartIndex) {
    std::string firstLine = fetchResponse.substr(pointer, fetchResponse.substr(pointer).find_first_of("\r\n"));
    std::string emailUID = firstLine.substr(2, firstLine.substr(2).find_first_of(" "));

    unsigned long emailSizeStart = firstLine.find_first_of("{");
    unsigned long emailSizeEnd = firstLine.find_first_of("}");
    if (emailSizeStart == std::string::npos || emailSizeEnd == std::string::npos) {
      throw std::runtime_error("Invalid fetch response format.");
    }
    int emailSize = std::stoi(firstLine.substr(emailSizeStart + 1, emailSizeEnd - emailSizeStart - 1));

    std::string email = fetchResponse.substr(pointer + emailSizeEnd + 3, emailSize);
    emails.insert({emailUID, email});

    pointer += emailSizeEnd + 3 + emailSize + 3;
  }

  return emails;
}

std::string IMAPClient::getNewEmailUIDs() {
  if (!this->isLoggedIn) {
    throw std::runtime_error("User must be logged in before fetching emails.");
  }

  // Send SEARCH command to server
  std::string command = std::to_string(this->tag) + " search new\r\n";
  std::string response = this->connection->sendCommand(this->tag, command);

  // Verify that searching emails was successful
  if (response.find(std::to_string(this->tag) + " OK") == std::string::npos) {
    throw std::runtime_error("Could not search emails.");
  }

  std::string firstLine = response.substr(0, response.find_first_of("\r\n"));
  if (firstLine.length() == 8) {
    return "";
  }
  std::string uids = response.substr(9, response.find_first_of("\r\n") - 9);
  for (char& character : uids) {
    if (character == ' ') {
      character = ',';
    }
  }

  this->tag++;
  return uids;
}

std::string IMAPClient::toLowerCase(std::string& input) {
  std::string output = input;
  std::transform(output.begin(), output.end(), output.begin(), [](unsigned char c) { return std::tolower(c); });

  return output;
}
