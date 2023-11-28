EXEC_NAME = jsh

CC = clang
CC_FLAGS = -Wall -DNDEBUG $(OPTFLAGS)
LDFLAGS = -lreadline -lhistory

all: build

format:
	clang-format -style=file -i $$(find ./src -name '*.c' -or -name '*.h')

build:
	clang-format -style=file --dry-run --Werror $$(find ./src -name '*.c' -or -name '*.h')
	$(CC) $(CC_FLAGS) $(INCLUDES) src/*.c -o $(EXEC_NAME) $(LDFLAGS)

run:
	./$(EXEC_NAME)
