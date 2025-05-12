LIB      := ../lib

CC       := gcc
# CFLAGS   += -std=c++11 -Wall -fpermissive -mrtm -g -O0 #-DUSE_REPLAYER # -DNDEBUG
CFLAGS   += -std=c++11 -Wall -fpermissive -mrtm -g -O2  #-DNDEBUG
CFLAGS   += -I $(LIB)
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     := -lpthread
# SRCS	 += ../include/hashmap.h

# Remove these files when doing clean
OUTPUT +=
