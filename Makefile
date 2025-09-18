CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lutil
TARGET = xlogger
SOURCE = xlogger.c

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) xlogger.log

install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

.DEFAULT_GOAL := all