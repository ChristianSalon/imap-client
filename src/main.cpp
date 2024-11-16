/**
 * @file main.cpp
 * @author Christian Saloň <xsalon02>
 */

#include <cstdint>
#include <exception>
#include <filesystem>
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

const std::string getAllOutputMessage(std::size_t count, std::string mailbox) {
  return "Downloaded " + std::to_string(count) + " emails from mailbox " + mailbox + ".";
}

const std::string getHeadersOutputMessage(std::size_t count, std::string mailbox) {
  return "Downloaded headers from " + std::to_string(count) + " emails from mailbox " + mailbox + ".";
}

const std::string getNewOutputMessage(std::size_t count, std::string mailbox) {
  return "Downloaded " + std::to_string(count) + " new emails from mailbox " + mailbox + ".";
}

const std::string getNewHeadersOutputMessage(std::size_t count, std::string mailbox) {
  return "Downloaded headers from " + std::to_string(count) + " new emails from mailbox " + mailbox + ".";
}

/**
 * @brief Convert a string to lower case
 *
 * @param input Input string
 * @return std::string Input string in lower case
 */
std::string toLowerCase(std::string input) {
  std::string output = input;
  std::transform(output.begin(), output.end(), output.begin(), [](unsigned char c) { return std::tolower(c); });

  return output;
}

/**
 * @brief Save emails to selected directory
 *
 * @param emails Pairs, where the key is the UID of an email and the value is the contents of the email
 * @param directoryPath Path where to save emails
 */
void deleteEmails(std::string hostname, std::string mailbox, std::string directoryPath) {
  // Check if email directory exists
  if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
    return;
  }

  for (const auto &file : std::filesystem::directory_iterator(directoryPath)) {
    if (file.is_regular_file()) {
      std::string filename = file.path().filename().string();

      // Check if email filename starts with hostname and mailbox
      if (filename.starts_with(hostname + "_" + mailbox)) {
        std::filesystem::remove(file.path());
      }
    }
  }
}

/**
 * @brief Save emails to selected directory
 *
 * @param emails Pairs, where the key is the UID of an email and the value is the contents of the email
 * @param directoryPath Path where to save emails
 */
void saveEmails(std::unordered_map<std::string, std::string> emails, std::string directoryPath) {
  for (std::pair<std::string, std::string> email : emails) {
    std::string outputFilePath = directoryPath + (directoryPath.ends_with("/") ? "" : "/") + email.first;
    std::ofstream outputFile{outputFilePath};
    outputFile.write(email.second.c_str(), email.second.length());
    outputFile.close();
  }
}

/**
 * @brief Entry point
 *
 * @param argc Command line argument count
 * @param argv Command line argument values
 * @return int Return code
 */
int main(int argc, char **argv) {
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
  bool interactiveMode = false;

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
    } else if (strcmp(argv[i], "-i") == 0) {
      interactiveMode = true;
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
    // Initialize imap client
    IMAPClient client = useSecure ? IMAPClient{serverAddress, port, certificateFilePath, certificatesDirectory}
                                  : IMAPClient{serverAddress, port};

    if (interactiveMode) {
      std::string input;
      while (true) {
        // Get input from user
        std::getline(std::cin, input);
        input = toLowerCase(input);

        // Parse user command
        if (input.starts_with("downloadall")) {
          std::string selectedMailbox = mailbox;
          if (input.length() >= 13) {
            // Select mailbox in user command
            selectedMailbox = input.substr(12);
          }

          // Select mailbox and fetch all emails
          client.select(selectedMailbox);
          std::unordered_map<std::string, std::string> emails =
              client.fetch(useOnlyHeaders ? IMAPClient::FetchOptions::HEADERS : IMAPClient::FetchOptions::ALL);
          // Delete emails that are in selected mailbox to ensure client is synced with server
          deleteEmails(serverAddress, selectedMailbox, outputDirectory);
          saveEmails(emails, outputDirectory);
          std::cout << (useOnlyHeaders ? getHeadersOutputMessage(emails.size(), mailbox)
                                       : getAllOutputMessage(emails.size(), mailbox))
                    << std::endl;
        } else if (input.starts_with("downloadnew")) {
          std::string selectedMailbox = mailbox;
          if (input.length() >= 13) {
            // Select mailbox in user command
            selectedMailbox = input.substr(12);
          }

          // Select mailbox and fetch new emails
          client.select(selectedMailbox);
          std::unordered_map<std::string, std::string> emails =
              client.fetchNew(useOnlyHeaders ? IMAPClient::FetchOptions::HEADERS : IMAPClient::FetchOptions::ALL);
          saveEmails(emails, outputDirectory);
          std::cout << (useOnlyHeaders ? getNewHeadersOutputMessage(emails.size(), mailbox)
                                       : getNewOutputMessage(emails.size(), mailbox))
                    << std::endl;
        } else if (input.starts_with("readnew")) {
          std::string selectedMailbox = mailbox;
          if (input.length() >= 9) {
            // Select mailbox in user command
            selectedMailbox = input.substr(8);
          }

          client.read();
          std::cout << "Emails in mailbox " << mailbox << " were read." << std::endl;
        } else if (input.starts_with("quit")) {
          break;
        } else if (input.starts_with("starttls")) {
          if (client.startTls()) {
            std::cout << "Started TLS." << std::endl;
          }
        } else if (input.starts_with("login")) {
          // Authenticate user
          client.login(username, password);
          std::cout << "Logged in user " << username << "." << std::endl;
        } else {
          std::cerr << "ERROR: Invalid command." << std::endl;
        }
      }
    } else {
      // Authenticate user
      client.login(username, password);

      // Fetch emails from server
      std::unordered_map<std::string, std::string> emails;
      client.select(mailbox);
      if (useOnlyNewMessages) {
        emails = client.fetchNew(useOnlyHeaders ? IMAPClient::FetchOptions::HEADERS : IMAPClient::FetchOptions::ALL);
      } else {
        // Delete emails that are in selected mailbox to ensure client is synced with server
        deleteEmails(serverAddress, mailbox, outputDirectory);

        emails = client.fetch(useOnlyHeaders ? IMAPClient::FetchOptions::HEADERS : IMAPClient::FetchOptions::ALL);
      }

      saveEmails(emails, outputDirectory);

      if (useOnlyNewMessages) {
        std::cout << (useOnlyHeaders ? getNewHeadersOutputMessage(emails.size(), mailbox)
                                     : getNewOutputMessage(emails.size(), mailbox))
                  << std::endl;
      } else {
        std::cout << (useOnlyHeaders ? getHeadersOutputMessage(emails.size(), mailbox)
                                     : getAllOutputMessage(emails.size(), mailbox))
                  << std::endl;
      }
    }
  } catch (const std::exception &e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
