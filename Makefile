BIN_NAME = jsh
BIN_DIRECTORY = bin
BIN_PATH = $(BIN_DIRECTORY)/$(BIN_NAME)

CC = clang
CC_FLAGS = -Wall

all: build run

format:
	clang-format -style=file -i $$(find ./src -name '*.c' -or -name '*.h')

build:
	mkdir -p $(BIN_DIRECTORY)
	clang-format -style=file --dry-run --Werror $$(find ./src -name '*.c' -or -name '*.h')
	$(CC) $(CC_FLAGS) src/*.c -o $(BIN_PATH)

run:
	$(BIN_PATH)
