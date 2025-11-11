CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
LDFLAGS = -lreadline  # NEW: Link with readline library
SRCDIR = src
INCDIR = include
BINDIR = bin
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/shell.c $(SRCDIR)/execute.c
TARGET = $(BINDIR)/myshell

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LDFLAGS)  # UPDATED: Added $(LDFLAGS)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)

test: $(TARGET)
	./$(TARGET)

# NEW: Install dependencies target
install-deps:
	sudo apt-get update
	sudo apt-get install libreadline-dev
