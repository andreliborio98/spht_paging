OBJS     := \
	src/input_handler.o \
#

LIBS     :=
INCS     := -I ./include
DEFS     :=

CC       := gcc
CXX      := g++ -std=c++14
AR       := ar rcs

DEBUG    ?= 0

ifeq ($(DEBUG),1)
DEFS     += -g -G
endif

LIB      := input_handler

CFLAGS   := -c $(DEFS) $(INCS)
CXXFLAGS := -c $(DEFS) $(INCS)

LDFLAGS  := $(LIBS) -L . -l $(LIB)

TEST_OBJS := \
	tests/main.o \
#

all: lib$(LIB).a main
	# DONE

lib$(LIB).a: $(OBJS)
	@echo "Linking..."
	$(AR) $@ $(OBJS)

main: tests/main.o
	$(CXX) $^ $(LDFLAGS) -o $@

%.o:	%.c
	@echo "--- $<"
	$(CC) $(CFLAGS) -o $@ $<

%.o:	%.cpp
	@echo "--- $<"
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(OBJS) $(TEST_OBJS) lib$(LIB).a
