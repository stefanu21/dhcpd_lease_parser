VERSION = 0.0
CC = gcc
CFLAGS = -Wall -g3 -DVERSION=\"$(VERSION)\"
LDFLAGS = 
BIN = lease_parser
OBJ = main.o dllist.o 

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ) $(LDFLAGS)

%.o:%.c
	$(CC) $(CFLAGS) -c $<


.PHONY: all clean

all:
	make $(BIN)
	
clean:
	rm -rf $(BIN) $(OBJ)

