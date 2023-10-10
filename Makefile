CFLAGS=-Wall -Wextra -Wpedantic
BIN:=tts
OBJ:=main.o

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

clean:
	rm -f $(BIN) $(OBJ)
