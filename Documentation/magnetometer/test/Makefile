###############################################################
#
# Purpose: Makefile for HMC5883L library
# Author.: Chip Wood
# Version: 0.1
# License: GPL
#
###############################################################

TARGET = compass

CC = g++

SOURCES = hmc5883l_test.cpp
OBJECTS=$(SOURCES:.cpp=.o)

$(TARGET): $(OBJECTS)
	$(CC) $(SOURCES) -o $@

clean:
	rm -rf $(OBJECTS) $(TARGET)
