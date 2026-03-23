CC := gcc
CFLAGS := -Wall -Wextra -Iinclude

SRC := src
INC := include

OBJ := build/obj
BIN := build/bin

SRC_CONSOLE = $(wildcard $(SRC)/console/*.c)
OBJ_CONSOLE = $(patsubst $(SRC)/console/%.c, $(OBJ)/console/%.o, $(SRC_CONSOLE))

all: console script

console: $(OBJ_CONSOLE)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -I$(INC)/console $^ -o $(BIN)/jms_console

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC)/$(dir $*) -c $< -o $@

script:
	@chmod +x $(SRC)/script/jms_script.sh

clean:
	rm -rf build/

.PHONY: all clean console script