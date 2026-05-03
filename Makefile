CC      = gcc
CFLAGS  = -Wall -Wextra -g -pthread
LDFLAGS = -lsqlite3 -lpthread

SRC_DIR = src
SRCS    = $(SRC_DIR)/main.c     \
					$(SRC_DIR)/server.c   \
					$(SRC_DIR)/client.c   \
					$(SRC_DIR)/database.c \
					$(SRC_DIR)/protocol.c

TARGET  = chat-server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) chat.db