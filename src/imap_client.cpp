/**
 * @file imap_client.cpp
 * @author Christian Saloň
 */

#include "imap_client.h"

/**
 * @brief Construct a new imap client which uses a ssl connection
 *
 * @param hostname Server hostname
 * @param port Server port
 * @param certificateFile Path to a certificate file used for validating ssl/tls certificate
 * @param certificatesFolderPath Path to a folder which is used for validating ssl/tls certificates
 */
IMAPClient::IMAPClient(std::string hostname,
                       uint16_t port,
                       std::string certificateFile,
                       std::string certificatesFolderPath) {
  // Create a connection to server
  connection = std::make_unique<SSLConnection>(hostname, port, certificateFile, certificatesFolderPath);

  // Receive server greeting
  this->connection->receive();
}

/**
 * @brief Construct a new imap client which uses a tcp connection
 *
 * @param hostname Server hostname
 * @param port Server port
 */
IMAPClient::IMAPClient(std::string hostname, uint16_t port) {
  // Create a connection to server
  connection = std::make_unique<TCPConnection>(hostname, port);

  // Receive server greeting
  this->connection->receive();
}

/**
 * @brief Destroy the imap client
 */
IMAPClient::~IMAPClient() {}

/**
 * @brief Authenticate a user by sending the LOGIN command to the server
 *
 * @param username Username used for authentication
 * @param password Password used for authentication
 */
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

/**
 * @brief Logout a user by sending the LOGOUT command to the server
 */
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

/**
 * @brief Select a mailbox by sending the SELECT command to the server
 *
 * @param mailbox Name of mailbox to select
 */
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

/**
 * @brief Get all emails in selected mailbox by sending the FETCH command to the server
 *
 * @param options Specify which email contents to fetch
 * @return std::unordered_map<std::string, std::string> Pairs, where the key is the UID of an email and the value is the
 * contents of the email
 */
std::unordered_map<std::string, std::string> IMAPClient::fetch(FetchOptions options) {
  // User must be logged in before fetching emails
  if (!this->isLoggedIn) {
    throw std::runtime_error("User must be logged in before fetching emails.");
  }

  // Selected mailbox must not be empty
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

/**
 * @brief Get only new emails in selected mailbox by sending the FETCH command to the server
 *
 * @param options Specify which email contents to fetch
 * @return std::unordered_map<std::string, std::string> Pairs, where the key is the UID of an email and the value is the
 * contents of the email
 */
std::unordered_map<std::string, std::string> IMAPClient::fetchNew(FetchOptions options) {
  // User must be logged in before fetching emails
  if (!this->isLoggedIn) {
    throw std::runtime_error("User must be logged in before fetching emails.");
  }

  // Selected mailbox must not be empty
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

/**
 * @brief Parses a FETCH response into a map of emails
 *
 * @param fetchResponse FETCH response sent from the server
 * @return std::unordered_map<std::string, std::string> Pairs, where the key is the UID of an email and the value is the
 * contents of the email
 */
std::unordered_map<std::string, std::string> IMAPClient::parseEmails(std::string fetchResponse) {
  std::unordered_map<std::string, std::string> emails;
  unsigned long pointer = 0;  // Position in the fetch response
  unsigned long lastRowStartIndex = fetchResponse.substr(0, fetchResponse.length() - 2).find_last_of("\r\n");

  while (pointer < lastRowStartIndex) {
    // Parse the first line of currently proccessed email
    std::string firstLine = fetchResponse.substr(pointer, fetchResponse.substr(pointer).find_first_of("\r\n"));
    std::string emailUID = firstLine.substr(2, firstLine.substr(2).find_first_of(" "));

    // Parse the email size
    unsigned long emailSizeStart = firstLine.find_first_of("{");
    unsigned long emailSizeEnd = firstLine.find_first_of("}");
    if (emailSizeStart == std::string::npos || emailSizeEnd == std::string::npos) {
      throw std::runtime_error("Invalid fetch response format.");
    }
    int emailSize = std::stoi(firstLine.substr(emailSizeStart + 1, emailSizeEnd - emailSizeStart - 1));

    // Parse the email content
    std::string email = fetchResponse.substr(pointer + emailSizeEnd + 3, emailSize);
    emails.insert({emailUID, email});

    pointer += emailSizeEnd + 3 + emailSize + 3;
  }

  return emails;
}

/**
 * @brief Get UIDs of emails that are new by sending a SEARCH command to the server
 *
 * @return std::string Sequence set representing UIDs of new emails
 */
std::string IMAPClient::getNewEmailUIDs() {
  // User must be logged in
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

  // Parse the response
  std::string firstLine = response.substr(0, response.find_first_of("\r\n"));
  if (firstLine.length() == 8) {
    return "";
  }
  std::string uids = response.substr(9, response.find_first_of("\r\n") - 9);

  // Replace all spaces with commas to create a valid sequence set
  for (char& character : uids) {
    if (character == ' ') {
      character = ',';
    }
  }

  this->tag++;
  return uids;
}

/**
 * @brief Convert a string to lower case
 *
 * @param input Input string
 * @return std::string Input string in lower case
 */
std::string IMAPClient::toLowerCase(std::string& input) {
  std::string output = input;
  std::transform(output.begin(), output.end(), output.begin(), [](unsigned char c) { return std::tolower(c); });

  return output;
}
