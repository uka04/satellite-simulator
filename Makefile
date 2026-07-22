CC = gcc
CXX = g++

CFLAGS = -Iinclude -Wall
CXXFLAGS = -Iinclude -Wall

LDFLAGS = -lm -lncurses

TARGET = satellite_sim

C_OBJS = main.o sensor.o
CPP_OBJS = sgp4_wrapper.o sgp4unit.o sgp4ext.o

all: $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) -o $(TARGET) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS)

main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c

sensor.o: src/sensor.c
	$(CC) $(CFLAGS) -c src/sensor.c

sgp4_wrapper.o: src/sgp4_wrapper.cpp
	$(CXX) $(CXXFLAGS) -c src/sgp4_wrapper.cpp

sgp4unit.o: src/sgp4unit.cpp
	$(CXX) $(CXXFLAGS) -c src/sgp4unit.cpp

sgp4ext.o: src/sgp4ext.cpp
	$(CXX) $(CXXFLAGS) -c src/sgp4ext.cpp

clean:
	rm -f *.o $(TARGET)
