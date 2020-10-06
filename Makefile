## Hubert Obrzut, lista nr 3 ##

TARGET = terminal
EXENAME = terminal

CC = gcc
CFLAGS = -D _GNU_SOURCE -Wall -Wextra -Wno-unused
DFLAGS = -fsanitize=address
OBJS = $(TARGET).o
LIBS = -lreadline

all: $(TARGET)

debug: CFLAGS += $(DFLAGS)
debug: $(TARGET)

install: $(TARGET) clean

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXENAME) $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o
distclean: clean
	rm -f $(EXENAME)

.PHONY: clean
