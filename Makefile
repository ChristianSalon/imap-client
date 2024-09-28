CXX = g++
CXXFLAGS = -std=c++20

EXECUTABLE = imapcl
SOURCES = src/main.cpp src/connection.cpp src/imap_client.cpp src/ssl_connection.cpp src/tcp_connection.cpp
HEADERS = src/connection.h src/imap_client.h src/ssl_connection.h src/tcp_connection.h

$(EXECUTABLE): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES)

clean:
	rm -f $(EXECUTABLE)