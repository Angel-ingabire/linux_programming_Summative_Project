# =============================================================================
# Makefile for Multithreaded Order Processing System
# =============================================================================
# Usage:
#   make        - Build the program
#   make run    - Build and run with default 20 orders
#   make run10  - Build and run with 10 orders
#   make run50  - Build and run with 50 orders
#   make clean  - Remove compiled binary
# =============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
TARGET = order_processing

.PHONY: all run run10 run50 clean

all: $(TARGET)

$(TARGET): order_processing.c
	$(CC) $(CFLAGS) -o $(TARGET) order_processing.c

run: $(TARGET)
	./$(TARGET)

run10: $(TARGET)
	./$(TARGET) 10

run50: $(TARGET)
	./$(TARGET) 50

clean:
	rm -f $(TARGET)

