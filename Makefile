CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude
SRCDIR = src
INCDIR = include
BINDIR = bin
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/shell.c $(SRCDIR)/execute.c
TARGET = $(BINDIR)/myshell

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES) | $(BINDIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

$(BINDIR):
	mkdir -p $(BINDIR)

clean:
	rm -rf $(BINDIR)

test: $(TARGET)
	./$(TARGET)
