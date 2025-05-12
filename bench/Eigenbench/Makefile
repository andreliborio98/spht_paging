MY_LIBS  := ../../deps
NVHTM    := ../../nvhtm

LIB_PATH := -L $(MY_LIBS)/input_handler \
	-L $(MY_LIBS)/threading -L $(NVHTM) \
	-L $(MY_LIBS)/htm_alg \
#
LIBS     := -l input_handler -l threading  -l htm_sgl -l nvhtm -pthread -mrtm

INCS     := -I include -I $(MY_LIBS)/input_handler/include -I $(MY_LIBS)/arch_dep/include \
  -I $(MY_LIBS)/threading/include -I $(MY_LIBS)/htm_alg/include -I $(NVHTM)/include \
#

### needs to define DEF_CPU_FREQ
DEF_CPU_FREQ=$(shell cat CPU_FREQ_kHZ.txt)
DEFS     := -DDEF_CPU_FREQ=$(DEF_CPU_FREQ)

CC       := gcc
CXX      := g++

DEBUG    ?= 0
OPTIMIZE ?= 0

ifeq ($(DEBUG),1)
DEFS     += -g
endif

ifeq ($(OPTIMIZE),1)
DEFS     += -O3
endif

OBJS := \
	src/eigenbench.o \
	src/main.o \
#

CFLAGS   := -c $(DEFS) $(INCS) -mrtm
CXXFLAGS := -c $(DEFS) $(INCS) -std=c++14 -mrtm

LDFLAGS  := $(LIBS)
LD       := $(CXX) $(LIB_PATH)

all: bench

bench: $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS)

%.o:	%.c ./include/tm_impl_nvhtm.h
	@echo "--- $<"
	$(CC) $(CFLAGS) -o $@ $<

%.o:	%.cpp ./include/tm_impl_nvhtm.h
	@echo "--- $<"
	$(CXX) $(CXXFLAGS) -o $@ $<

./include/tm_impl_nvhtm.h:
	cp ./include/tm_impls/tm_impl_nvhtm.h ./include/tm_impl.h
	cp ./include/tm_impls/tm_impl_nvhtm.h ./include/tm_impl_nvhtm.h

clean:
	rm -f $(OBJS) bench ./include/tm_impl_nvhtm.h
