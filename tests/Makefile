CC  := gcc

CFLAGS  := -O3 -D_GNU_SOURCE
#CFLAGS  := -O0 -ggdb -D_GNU_SOURCE
LDFLAGS := -pthread

TARGET := replay-st replay-nt replay-dram replay-wbinvd replay-allf

SOURCES := main.c log-generate.c log-replay.c

ALL: $(TARGET) 

replay-st: $(SOURCES)
	  $(CC) $(CFLAGS) -DUSE_FLUSH_PER_STORE $^ -o $@  $(LDFLAGS)

replay-nt: $(SOURCES)
	  $(CC) $(CFLAGS) -DUSE_NT_STORE $^ -o $@  $(LDFLAGS)

replay-dram: $(SOURCES)
	  $(CC) $(CFLAGS) -DUSE_DRAM $^ -o $@  $(LDFLAGS)

replay-allf: $(SOURCES)
	  $(CC) $(CFLAGS) -DBATCH_FLUSH $^ -o $@  $(LDFLAGS)

replay-wbinvd: $(SOURCES)
	  $(CC) $(CFLAGS) -DUSE_WBINVD $^ -o $@  $(LDFLAGS)

clean:
	  rm -f $(TARGET) *.o 
