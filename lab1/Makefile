
CC = gcc
CXX = g++ -std=c++11
CC_EXTRA_FLAGS = -O0 -g3  -Wall
CC_ERR_FLAGS = -Wno-format
CC_FLAGS = -I./ $(CC_ERR_FLAGS) $(CC_EXTRA_FLAGS)

CC_FILES = $(wildcard *.c)
CXX_FILES = $(wildcard *.cc)

# Compile C files to object files.
%.o :: %.c
	@echo "Building C object $@"
	@mkdir -p $(@D)
	@$(CC) $(CC_FLAGS) -Dsbrk=DoSbrk -c $< -o $@

# Compile C++ files to object files.
%.o :: %.cc
	@echo "Building C object $@"
	@mkdir -p $(@D)
	@$(CXX) $(CC_FLAGS) -Dsbrk=DoSbrk -c $< -o $@

all: $(CC_FILES:.c=.o) $(CXX_FILES:.cc=.o)
	@echo "Building $@"
	@$(CXX) -o a.out -pthread -lm $^

clean:
	@-rm $(wildcard *.o) a.out > /dev/null 2>&1 ||:
