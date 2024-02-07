CFLAGS=-Wall -Wextra -Wpedantic -O2
BIN:=tts
OBJ:=main.o

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $(BIN) $(OBJ)

clean:
	rm -f $(BIN) $(OBJ)

.PHONY: $(BIN) clean
