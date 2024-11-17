/**
 * IMAP client
 *
 * @file connection.cpp
 * @author Christian Saloň <xsalon02>
 */

#include "connection.h"

/**
 * @brief Validates that the response from the server is correctly formated
 *
 * @param response Response from the server
 * @param tag Tag of sent command to server
 * @return true If response is valid
 * @return false If response is not valid
 */
bool Connection::isResponseFull(std::string response, unsigned int tag) {
  if (response.length() == 0) {
    return false;
  }

  unsigned long lastRowStartIndex = response.substr(0, response.length() - 2).find_last_of("\r\n");
  if (lastRowStartIndex == std::string::npos) {
    lastRowStartIndex = 0;
  }

  std::string tagString = std::to_string(tag);
  if (response.starts_with(tagString) || response.substr(lastRowStartIndex + 1).starts_with(tagString)) {
    return true;
  }

  return false;
}
