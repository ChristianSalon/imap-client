CXX = g++
CXXFLAGS = -std=c++20
LDFLAGS = -lssl -lcrypto

EXECUTABLE = imapcl
SOURCES = src/main.cpp src/connection.cpp src/imap_client.cpp src/ssl_connection.cpp src/tcp_connection.cpp
HEADERS = src/connection.h src/imap_client.h src/ssl_connection.h src/tcp_connection.h

TAR_NAME = xsalon02.tar

$(EXECUTABLE): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

pack:
	tar -cf $(TAR_NAME) $(SOURCES) ${HEADERS} Makefile README manual.pdf

clean:
	rm -f $(EXECUTABLE) $(TAR_NAME)

.PHONY: pack clean
