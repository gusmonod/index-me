ZIPF_SRC=$(wildcard zipf/*.c) $(wildcard util/*.c)
CONS_SRC=$(wildcard construct/*.c) $(wildcard util/*.c)
MERG_SRC=$(wildcard merger/*.c) $(wildcard util/*.c)

ZIPF_OBJECTS=$(addprefix obj/, $(ZIPF_SRC:.c=.o))
CONS_OBJECTS=$(addprefix obj/, $(CONS_SRC:.c=.o))
MERG_OBJECTS=$(addprefix obj/, $(MERG_SRC:.c=.o))

CC=gcc
CC_FLAGS= -std=c99 -pedantic -Wall -W -Wextra
TARGET ?= debug

ifeq ($(TARGET), release)
		CC_FLAGS +=-DNDEBUG -O3
else
		CC_FLAGS +=-g -DDEBUG
endif

LD=gcc
LD_FLAGS=

LIBS=-lm
LIB_PATH=-L/usr/local/lib

INC=
INC_PATH=-I$(shell pwd)

.PHONY: clean test corpus merger

# aliases
all: construct merger zipf
construct: bin/construct
merger: bin/merger
zipf: bin/zipf

corpus:
	@mkdir -p corpus
	(cd tokenizer && npm install && node node_modules/.bin/gulp build-corpus)

bin/construct: $(CONS_OBJECTS)
	@mkdir -p bin
	$(LD) $(CC_FLAGS) $(LD_FLAGS) $(INC_PATH) $(LIB_PATH) \
	      $(CONS_OBJECTS) $(LIBS) -o $@
	@mv bin/construct.exe bin/construct 2> /dev/null || test 1

bin/merger: $(MERG_OBJECTS)
	@mkdir -p bin
	$(LD) $(CC_FLAGS) $(LD_FLAGS) $(INC_PATH) $(LIB_PATH) \
	      $(MERG_OBJECTS) $(LIBS) -o $@
	@mv bin/merger.exe bin/merger 2> /dev/null || test 1

bin/zipf: $(ZIPF_OBJECTS)
	@echo $@: not implemented yet
	@mv bin/zipf.exe bin/zipf 2> /dev/null || test 1

obj/%.o: %.c
	@mkdir -p $(shell echo $@ | sed 's/[^/]*$$//g')
	@echo "Compiling $<..."
	$(CC) $(CC_FLAGS) $(INC_PATH) -c $< -o $@
	@echo "... Compiled $<\n"

clean:
	rm -rf obj/* bin/* core tokenizer/node_modules tokenizer/obj

test: bin/construct
	(cd test; ./mktest.sh)
