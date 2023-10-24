CC=gcc
CFLAGS=-I.
DEPS = config.h
OBJ = main.c config.c

%.o: %.c $(DEPS)
		$(CC) -c -o $@ $< $(CFLAGS)

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)