CC = clang++

# compiler flags:
#  -g     - this flag adds debugging information to the executable file
#  -Wall  - this flag is used to turn on most compiler warnings
CFLAGS  = -std=c++20 -g -Wall -I/usr/local/include -I/** -L/usr/local/lib -I/usr/include/libxml2 -lcurlpp -lcurl -lxml2

# The build target 
TARGET = main

all: $(TARGET)

build: $(TARGET).cpp
			$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp

clean:
			$(RM) $(TARGET)