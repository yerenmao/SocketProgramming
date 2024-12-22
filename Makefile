# Compiler and flags
CXX = g++
CXXFLAGS = -Wall -Wextra -pthread -std=c++17

# Directories
CLIENT_DIR = client
SERVER_DIR = server
SHARED_DIR = shared

# Target executables
CLIENT_TARGET = client_app
SERVER_TARGET = server_app

# Shared library sources and objects
SHARED_SOURCES = $(wildcard $(SHARED_DIR)/*.cpp)
SHARED_OBJECTS = $(SHARED_SOURCES:.cpp=.o)

# Client sources and objects
CLIENT_SOURCES = $(wildcard $(CLIENT_DIR)/*.cpp)
CLIENT_OBJECTS = $(CLIENT_SOURCES:.cpp=.o)

# Server sources and objects
SERVER_SOURCES = $(wildcard $(SERVER_DIR)/*.cpp)
SERVER_OBJECTS = $(SERVER_SOURCES:.cpp=.o)

# Default rule: build both client and server
all: $(CLIENT_TARGET) $(SERVER_TARGET)

# Build client
$(CLIENT_TARGET): $(SHARED_OBJECTS) $(CLIENT_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_TARGET) $(SHARED_OBJECTS) $(CLIENT_OBJECTS)

# Build server
$(SERVER_TARGET): $(SHARED_OBJECTS) $(SERVER_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(SERVER_TARGET) $(SHARED_OBJECTS) $(SERVER_OBJECTS)

# Compile shared sources to object files
$(SHARED_DIR)/%.o: $(SHARED_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile client sources to object files
$(CLIENT_DIR)/%.o: $(CLIENT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile server sources to object files
$(SERVER_DIR)/%.o: $(SERVER_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(CLIENT_OBJECTS) $(SERVER_OBJECTS) $(SHARED_OBJECTS) $(CLIENT_TARGET) $(SERVER_TARGET)

# Phony targets
.PHONY: all clean client server

# Build only client
client: $(CLIENT_TARGET)

# Build only server
server: $(SERVER_TARGET)
