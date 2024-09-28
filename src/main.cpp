/**
 * @file main.cpp
 * @author Christian Saloň
 */

#include <cstdint>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include <string.h>

#include "imap_client.h"

const uint16_t IMAP_PORT = 143;
const uint16_t IMAPS_PORT = 993;
const std::string DEFAULT_CERTIFICATES_DIRECTORY = "/etc/ssl/certs";
const std::string DEFAULT_MAILBOX = "INBOX";

int main(int argc, char** argv) {
  std::string serverAddress;
  uint16_t port = IMAP_PORT;
  bool isPortSet = false;
  bool useSecure = false;
  std::string certificateFilePath;
  std::string certificatesDirectory = DEFAULT_CERTIFICATES_DIRECTORY;
  bool useOnlyNewMessages = false;
  bool useOnlyHeaders = false;
  std::string authFilePath;
  std::string mailbox = DEFAULT_MAILBOX;
  std::string outputDirectory;

  // Proccess command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0) {
      port = atoi(argv[++i]);
      isPortSet = true;
    } else if (strcmp(argv[i], "-T") == 0) {
      useSecure = true;

      if (!isPortSet) {
        port = IMAPS_PORT;
      }
    } else if (strcmp(argv[i], "-c") == 0) {
      certificateFilePath = argv[++i];
    } else if (strcmp(argv[i], "-C") == 0) {
      certificatesDirectory = argv[++i];
    } else if (strcmp(argv[i], "-n") == 0) {
      useOnlyNewMessages = true;
    } else if (strcmp(argv[i], "-h") == 0) {
      useOnlyHeaders = true;
    } else if (strcmp(argv[i], "-a") == 0) {
      authFilePath = argv[++i];
    } else if (strcmp(argv[i], "-b") == 0) {
      mailbox = argv[++i];
    } else if (strcmp(argv[i], "-o") == 0) {
      outputDirectory = argv[++i];
    } else {
      serverAddress = argv[i];
    }
  }

  // Check if required command line arguments are set
  if (serverAddress.empty() || authFilePath.empty() || outputDirectory.empty()) {
    std::cerr << "How to run the program: ./imapcl server [-p port] [-T [-c certfile] [-C certaddr]] [-n] [-h] -a "
                 "auth_file [-b MAILBOX] -o out_dir"
              << std::endl;
    return 1;
  }

  // Get credentails from auth file
  std::ifstream authFile{authFilePath};
  std::string line;

  // Parse username from auth file
  getline(authFile, line);
  if (line.substr(0, 11) != "username = ") {
    std::cerr << "ERROR: " << "Invalid username in auth file." << std::endl;
    return 1;
  }
  std::string username = line.substr(11);

  // Parse password from auth file
  getline(authFile, line);
  if (line.substr(0, 11) != "password = ") {
    std::cerr << "ERROR: " << "Invalid password in auth file." << std::endl;
    return 1;
  }
  std::string password = line.substr(11);

  // Close auth file
  authFile.close();

  try {
    IMAPClient client = useSecure ? IMAPClient{serverAddress, port, certificateFilePath, certificatesDirectory}
                                  : IMAPClient{serverAddress, port};
    client.login(username, password);

    // Fetch emails from server
    std::unordered_map<std::string, std::string> emails;
    client.select(mailbox);
    if (useOnlyHeaders) {
      if (useOnlyNewMessages) {
        emails = client.fetchNew(IMAPClient::FetchOptions::HEADERS);
      } else {
        emails = client.fetch(IMAPClient::FetchOptions::HEADERS);
      }
    } else {
      if (useOnlyNewMessages) {
        emails = client.fetchNew(IMAPClient::FetchOptions::ALL);
      } else {
        emails = client.fetch(IMAPClient::FetchOptions::ALL);
      }
    }

    // Write emails to output directory
    for (std::pair<std::string, std::string> email : emails) {
      std::string outputFilePath = outputDirectory + (outputDirectory.ends_with("/") ? "" : "/") + email.first;
      std::ofstream outputFile{outputFilePath};
      outputFile.write(email.second.c_str(), email.second.length());
      outputFile.close();
    }

    // Write email count to standard output
    std::cout << "Downloaded " << emails.size() << " emails from mailbox " << mailbox << std::endl;

    // Close connection to server
    client.logout();
  } catch (const std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
