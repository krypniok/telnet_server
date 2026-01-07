CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lsqlite3 -lm -lpthread

TARGET = server
SRCS = server.c app.c db.c gui.c email.c screen_login.c screen_intro.c screen_chat.c screen_dmesg.c screen_cube.c screen_demo.c screen_debug.c screen_poker.c screen_admin.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

%.o: %.c common.h gui.h db.h app.h email.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)