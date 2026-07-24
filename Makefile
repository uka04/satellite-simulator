CC = gcc
CXX = g++

CFLAGS = -Wall -Wextra -Iinclude -O2
CXXFLAGS = -Wall -Wextra -Iinclude -O2
LDFLAGS = -lm -lncurses

BIN_DIR = bin
LOG_DIR = logs

SERVER_BIN = $(BIN_DIR)/satellite_server
CLIENT_BIN = $(BIN_DIR)/satellite_client

SGP4_SRCS   = src/sgp4/sgp4ext.cpp src/sgp4/sgp4unit.cpp src/sgp4/sgp4_wrapper.cpp
COMMON_SRCS = src/common/sensor.c

SERVER_SRCS = src/server/server_main.c $(COMMON_SRCS)
CLIENT_SRCS = src/client/client_main.c $(COMMON_SRCS)

SGP4_OBJS   = $(SGP4_SRCS:.cpp=.o)
SERVER_OBJS = $(SERVER_SRCS:.c=.o) $(SGP4_OBJS)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)

all: create_dirs $(SERVER_BIN) $(CLIENT_BIN)

create_dirs:
	@mkdir -p $(BIN_DIR) $(LOG_DIR)

$(SERVER_BIN): $(SERVER_OBJS)
	$(CXX) $^ -o $@ $(LDFLAGS)
	@echo "Server Build Complete: $@"

$(CLIENT_BIN): $(CLIENT_OBJS)
	$(CC) $^ -o $@ $(LDFLAGS)
	@echo "Client Build Complete: $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o src/*/*.o src/sgp4/*.o
	rm -rf $(BIN_DIR)
	@echo "Cleaned all object and binary files."

.PHONY: all clean create_dirs
