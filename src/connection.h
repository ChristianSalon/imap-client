/**
 * @file connection.h
 * @author Christian Saloň <xsalon02>
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>

/**
 * @brief Represents a connection to a server
 */
class Connection {
 public:
  virtual std::string sendCommand(unsigned int tag, std::string command) = 0;
  virtual std::string receive() = 0;

  bool isResponseFull(std::string response, unsigned int tag);
};

#endif
