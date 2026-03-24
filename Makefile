CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -g

SRC := src
INC := include

OBJ := build/obj
BIN := build/bin

SRC_COORD = $(wildcard $(SRC)/coord/*.c)
OBJ_COORD = $(patsubst $(SRC)/coord/%.c, $(OBJ)/coord/%.o, $(SRC_COORD))

SRC_CONSOLE = $(wildcard $(SRC)/console/*.c)
OBJ_CONSOLE = $(patsubst $(SRC)/console/%.c, $(OBJ)/console/%.o, $(SRC_CONSOLE))

SRC_SCRIPT = $(SRC)/script/script.sh
SH_SCRIPT = $(BIN)/jms_script.sh

all: coord console script

coord: $(OBJ_COORD)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -I$(INC)/coord $^ -o $(BIN)/jms_coord

console: $(OBJ_CONSOLE)
	@mkdir -p $(BIN)
	$(CC) $(CFLAGS) -I$(INC)/console $^ -o $(BIN)/jms_console

$(OBJ)/%.o: $(SRC)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC)/$(dir $*) -c $< -o $@

script: $(SRC_SCRIPT)
	@cp $^ $(SH_SCRIPT)
	@chmod +x $(SH_SCRIPT)

clean:
	rm -rf build/

.PHONY: all clean coord console script