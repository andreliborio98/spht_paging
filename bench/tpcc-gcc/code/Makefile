WARNINGS = -w

# gcc flags used for both debug and opt builds
BASE_CXX_FLAGS := -MD -I.. $(CXXFLAGS) $(WARNINGS)

# Debug flags
# CXXFLAGS = -g $(BASE_CXX_FLAGS)
# Optimization flags
CXXFLAGS = -g -O3 -std=gnu++11 -DNDEBUG $(BASE_CXX_FLAGS)

# Link withthe C++ standard library
LDFLAGS=-lstdc++

BINARIES = btree_test randomgenerator_test tpccclient_test tpcctables_test tpccgenerator_test tpcc

all: $(BINARIES)

btree_test: btree_test.o stupidunit.o
randomgenerator_test: randomgenerator_test.o randomgenerator.o stupidunit.o
tpccclient_test: tpccclient_test.o tpccclient.o randomgenerator.o stupidunit.o
tpcctables_test: tpcctables_test.o tpcctables.o tpccdb.o randomgenerator.o stupidunit.o
tpccgenerator_test: tpccgenerator_test.o tpccgenerator.o tpcctables.o tpccdb.o randomgenerator.o stupidunit.o
tpcc: tpcc.o tpccclient.o tpccgenerator.o tpcctables.o tpccdb.o clock.o randomgenerator.o stupidunit.o

clean :
	$(RM) *.o *.d $(BINARIES)

-include *.d
