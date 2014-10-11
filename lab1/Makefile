
CC = clang
CC_EXTRA_FLAGS = -O0 -g3  -Wall -pedantic 
CC_ERR_FLAGS = -Wno-empty-translation-unit -Wno-c11-extensions
CC_FLAGS = -I./ $(CC_ERR_FLAGS) $(CC_EXTRA_FLAGS)

CC_FILES = $(wildcard *.c)

# Compile C files to object files.
%.o :: %.c
	@echo "Building C object $@"
	@mkdir -p $(@D)
	@$(CC) $(CC_FLAGS) -c $< -o $@

all: $(CC_FILES:.c=.o)
	@echo "Building $@"
	@$(CC) -o a.out -lpthread -lm $^

clean:
	-rm $(wildcard *.o)
