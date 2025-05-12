// Copyright 2008,2009,2010 Massachusetts Institute of Technology.
// All rights reserved. Use of this source code is governed by a
// BSD-style license that can be found in the LICENSE file.

#define __STDC_FORMAT_MACROS
#include <climits>
#include <cstdio>
#include <inttypes.h>
#include <getopt.h>
#include <pthread.h>

#include "clock.h"
#include "randomgenerator.h"
#include "tpccclient.h"
#include "tpccgenerator.h"
#include "tpcctables.h"
#include "tm.h"
#include "random.h"
#include "rdtsc.h"

#ifdef USE_PAGING
  #include "paging.h"
#endif

#define DEFAULT_STOCK_LEVEL_TXS_RATIO       4
#define DEFAULT_DELIVERY_TXS_RATIO          4
#define DEFAULT_ORDER_STATUS_TXS_RATIO      4
#define DEFAULT_PAYMENT_TXS_RATIO           43
#define DEFAULT_NEW_ORDER_TXS_RATIO         45
#define DEFAULT_NUMBER_WAREHOUSES           64
#define DEFAULT_TIME_SECONDS                10
#define DEFAULT_NUM_CLIENTS					        1
#define DEFAULT_HEAP_MMAP                   4294967296 //4gb
#define DEFAULT_NUM_TXS                     200000

int duration_secs;
long duration_txs;
int num_clients = 1;
long sum_txs_exec_client = 0;

extern volatile int TM_isSequential;
extern volatile int TM_totNbThreads;

unsigned int htm_rot_enabled = 1;
unsigned int allow_rots_ros = 1;
unsigned int allow_htms = 1;
unsigned int allow_stms = 0;

// typedef __attribute__((aligned(CACHE_LINE_SIZE))) long padded_scalar_t;
// __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t exists_sw;

// __thread unsigned long backoff = MIN_BACKOFF;
__thread unsigned long cm_seed = 123456789UL;

// __attribute__((aligned(CACHE_LINE_SIZE))) padded_statistics_t stats_array[80];

__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t single_global_lock = 0;
__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t fallback_in_use = 0;


// __attribute__((aligned(CACHE_LINE_SIZE))) padded_scalar_t counters[80];

__attribute__((aligned(CACHE_LINE_SIZE))) pthread_spinlock_t writers_lock = 0;

__thread unsigned int local_exec_mode = 1;

__thread unsigned int local_thread_id;

__thread long rs_mask_2 = 0xffffffffffff0000;
__thread long rs_mask_4 = 0xffffffff00000000;
__thread long offset = 0;
__thread char* p;
__thread int* ip;
__thread int16_t* i2p;
__thread long moffset = 0;
__thread long moffset_2 = 0;
__thread long moffset_6 = 0;

__thread void* rot_readset[1024];
__thread char crot_readset[8192];
__thread int irot_readset[2048];
__thread int16_t i2rot_readset[4096];

__thread unsigned long rs_counter = 0;


unsigned int ucb_levers = 3;
unsigned long ucb_trials[3];
unsigned long total_trials;

// moved here
static long int num_warehouses;

static TPCCTables* tables;
static SystemClock* local_clock;
// Create a generator for filling the database.
static tpcc::RealRandomGenerator* local_random;
static tpcc::NURandC cLoad;
static TPCCClient** clients;
//////////

int global_num_threads = 0;
pthread_rwlock_t rw_lock;


void warmup(void *data) {
	
  TM_THREAD_ENTER();
  local_thread_id = thread_getId();
	TPCCClient* client = (TPCCClient*)((TPCCClient**) data)[local_thread_id];
	SystemClock* clock = new SystemClock();
	int64_t begin = clock->getMicroseconds();
	int64_t beginCycles = begin * 2300;

  int64_t warmup_secs = 30;

  int64_t duration_usecs = warmup_secs * 1000000;
  int64_t duration_cycles = duration_usecs * 2300;
  int64_t end_cycles = duration_cycles + beginCycles;
	do {
		  client->doOne(TM_ARG_ALONE);
	} while (rdtsc() < end_cycles);

	TM_THREAD_EXIT();
}

// set return type to void from void*
void client(void *data) {
	TM_THREAD_ENTER();
  local_thread_id = thread_getId();
	TPCCClient* client = (TPCCClient*)((TPCCClient**) data)[local_thread_id];
	SystemClock* clock = new SystemClock();
	int64_t begin = clock->getMicroseconds();
	int64_t beginCycles = begin * 2300;
  int64_t duration_usecs = duration_secs * 1000000;
  int64_t duration_cycles = duration_usecs * 2300;
  int64_t end_cycles = duration_cycles + beginCycles;
	do {
		  client->doOne(TM_ARG_ALONE);
  #ifndef USE_TXS
    // } while (((clock->getMicroseconds() - begin)) < duration_usecs);
	} while (rdtsc() < end_cycles);
  #else
    // printf ("duration txs\n");
    for (int c = 0; c < num_clients; c++) { //num_clients
      sum_txs_exec_client += clients[c]->executed_stock_level_txs_;
      sum_txs_exec_client += clients[c]->executed_delivery_txs_;
      sum_txs_exec_client += clients[c]->executed_order_status_txs_;
      sum_txs_exec_client += clients[c]->executed_payment_txs_;
      sum_txs_exec_client += clients[c]->executed_new_order_txs_;
    }
  } while (sum_txs_exec_client <= duration_txs); 
  printf ("numclients: %d, duration_txs: %ld\n", num_clients, sum_txs_exec_client);
  #endif

// gets the total PM consumed - must be called in the context of the thread
// since thread local variables are used internally to the allocator
  client->total_pm_consumed_ = get_local_memory_used();

	TM_THREAD_EXIT();
}

void init_warehouse(void *data)
{
	TM_isSequential = 1;
	TM_THREAD_ENTER();
	// GLOBAL_instrument_write = 0;

	// Generate the data
	printf("Loading %ld warehouses... \n", num_warehouses);
	fflush(stdout);
	char now[Clock::DATETIME_SIZE+1];
	local_clock->getDateTimestamp(now);
	printf("num items: %d\n", Item::NUM_ITEMS);
  fflush(stdout);
	int64_t begin = local_clock->getMicroseconds();
	int ro = 1;
	// TM_BEGIN_NO_LOG();
	local_exec_mode = 2;
	TPCCGenerator generator(local_random, now, Item::NUM_ITEMS, District::NUM_PER_WAREHOUSE,
					Customer::NUM_PER_DISTRICT, NewOrder::INITIAL_NUM_PER_DISTRICT);
	generator.makeItemsTableSingleThread(tables);
	for (int i = 0; i < num_warehouses; ++i) {
			generator.makeWarehouseSingleThread(tables, i+1);
	}
	// TM_END_NO_LOG();
	int64_t end = local_clock->getMicroseconds();
	printf("%ld ms\n", (end - begin + 500)/1000);
  fflush(stdout);

	// GLOBAL_instrument_write = 1;

	TM_THREAD_EXIT();
	TM_isSequential = 0;
}

int main(int argc, char** argv) {
	int64_t begin, end;

	rw_lock = PTHREAD_RWLOCK_INITIALIZER;

    /*if (argc < 9) {
        printf("Please provide all the minimum parameters\n");
        exit(1);
    }*/

    struct option long_options[] = {
      // These options don't set a flag
      {"stockLevel transactions ratio",     required_argument, NULL, 's'},
      {"delivery transactions ratio",       required_argument, NULL, 'd'},
      {"order status transactions ratio",   required_argument, NULL, 'o'},
      {"payment transactions ratio",        required_argument, NULL, 'p'},
      {"new order txs ratio",               required_argument, NULL, 'r'},
      {"number warehouses",                 required_argument, NULL, 'w'},
      #ifndef USE_TXS
      {"duration in seconds",               required_argument, NULL, 't'},
      #else
      {"duration in transactions",          required_argument, NULL, 't'},
      #endif
      {"number of clients",		              required_argument, NULL, 'n'},
      {"workload change",                   required_argument, NULL, 'c'},
      {"maximum number of warehouses",      required_argument, NULL, 'm'},
      {"size of dram",                      required_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}
    };

    global_stock_level_txs_ratio = DEFAULT_STOCK_LEVEL_TXS_RATIO;
    global_delivery_txs_ratio = DEFAULT_DELIVERY_TXS_RATIO;
    global_order_status_txs_ratio = DEFAULT_ORDER_STATUS_TXS_RATIO;
    global_payment_txs_ratio = DEFAULT_PAYMENT_TXS_RATIO;
    global_new_order_ratio = DEFAULT_NEW_ORDER_TXS_RATIO;
    num_warehouses = DEFAULT_NUMBER_WAREHOUSES;
    duration_secs = DEFAULT_TIME_SECONDS;
    duration_txs = DEFAULT_NUM_TXS;
    num_clients = DEFAULT_NUM_CLIENTS;
    uint64_t heap_mmap = DEFAULT_HEAP_MMAP; //dram

    // If "-c" is found, then we start parsing the parameters into the workload_changes.
    // The argument of each "-c" is the number of seconds that it lasts.
    std::vector<int> workload_changes;
    int adapt_workload = 0;

    int i, c;
    while(1) {
		  i = 0;
		  c = getopt_long(argc, argv, "s:d:o:p:r:w:t:n:c:m:h:", long_options, &i);
		  if(c == -1)
		    break;

		  if(c == 0 && long_options[i].flag == 0)
		    c = long_options[i].val;

		switch(c) {
		   case 'c':
		      adapt_workload = 1;
		      workload_changes.push_back(atoi(optarg));
		    break;
		   case 's':
			 global_stock_level_txs_ratio = atoi(optarg);
		       workload_changes.push_back(atoi(optarg));
		     break;
		   case 'd':
			 global_delivery_txs_ratio = atoi(optarg);
		       workload_changes.push_back(atoi(optarg));
		     break;
		   case 'o':
			 global_order_status_txs_ratio = atoi(optarg);
		       workload_changes.push_back(atoi(optarg));
		     break;
		   case 'p':
			 		global_payment_txs_ratio = atoi(optarg);
		       workload_changes.push_back(atoi(optarg));
		     break;
		   case 'r':
			 global_new_order_ratio = atoi(optarg);
		       workload_changes.push_back(atoi(optarg));
		     break;
		   case 'w':
			 global_num_warehouses = atoi(optarg);
		       workload_changes.push_back(atoi(optarg));
		     break;
		   case 'm':
		       num_warehouses = atoi(optarg);
		       break;
		   case 't':
          #ifndef USE_TXS
		        duration_secs = atoi(optarg);
          #else
            duration_txs = atoi(optarg);
          #endif
		     break;
		   case 'n':
		  	 num_clients = atoi(optarg);
		  	 break;
       case 'h':
		  	 heap_mmap = atol(optarg);
		  	 break;
		   default:
		     printf("Incorrect argument! :(\n");
		     exit(1);
	  }
	}

  // THIS BENCHMARK SUCKS!
  // uint64t size_mmap = 0;
  SIM_GET_NUM_CPU(num_clients);
  TM_STARTUP(num_clients);
  // printf ("priv: %ld, SHARED: %ld\n", SIZE_OF_PRIV, SIZE_OF_SHARED);
  // fflush(stdout);

  P_MEMORY_STARTUP(num_clients);
  thread_startup(num_clients);

  tables = new TPCCTables();
  local_clock = new SystemClock();

  // Create a generator for filling the database.
  local_random = new tpcc::RealRandomGenerator();
  cLoad = tpcc::NURandC::makeRandom(local_random);
  local_random->setC(cLoad);

  // P_MEMORY_SHUTDOWN();
  // thread_shutdown();

  // SIM_GET_NUM_CPU(num_clients);
  // TM_STARTUP(1); // I think this part is single threaded
  // P_MEMORY_STARTUP(1);
  // thread_startup(1);

  // I think this is init
  // thread_start(init_warehouse, clients);

  TM_totNbThreads = num_clients;
  init_warehouse(clients);

  // Client owns all the parameters
  clients = (TPCCClient**) S_MALLOC(num_clients * sizeof(TPCCClient*));
  // pthread_t* threads = (pthread_t*) S_MALLOC(num_clients * sizeof(pthread_t));
  for (c = 0; c < num_clients; c++) {
    // Change the constants for run
    local_random = new tpcc::RealRandomGenerator();
    local_random->setC(tpcc::NURandC::makeRandomForRun(local_random, cLoad));
        clients[c] = new TPCCClient(local_clock, local_random, tables, Item::NUM_ITEMS, static_cast<int>(num_warehouses),
                District::NUM_PER_WAREHOUSE, Customer::NUM_PER_DISTRICT);
  }

  int64_t next_workload_secs;
  uint64_t pos_vec = 0;
  if (adapt_workload) {
    next_workload_secs = workload_changes[pos_vec++];
  } else {
    next_workload_secs = duration_secs;
  }

  /*global_num_warehouses = workload_changes[pos_vec++];
  global_stock_level_txs_ratio = workload_changes[pos_vec++];
  global_delivery_txs_ratio = workload_changes[pos_vec++];
  global_order_status_txs_ratio = workload_changes[pos_vec++];
  global_payment_txs_ratio = workload_changes[pos_vec++];
  global_new_order_ratio = workload_changes[pos_vec++];*/

  printf("------------------------------------------------------------------\n");
  printf("Running with the following parameters for %ld secs: (max warehouses %li)\n", next_workload_secs, num_warehouses);
  printf("\tWarehouses     (-w): %d\n", global_num_warehouses);
  printf("\tStockLevel ratio   (-s): %d\n", global_stock_level_txs_ratio);
  printf("\tDelivery ratio     (-d): %d\n", global_delivery_txs_ratio);
  printf("\tOrder Status ratio (-o): %d\n", global_order_status_txs_ratio);
  printf("\tPayment ratio      (-p): %d\n", global_payment_txs_ratio);
  printf("\tNewOrder ratio     (-r): %d\n", global_new_order_ratio);
  printf("------------------------------------------------------------------\n");

  int sum = global_stock_level_txs_ratio + global_delivery_txs_ratio + global_order_status_txs_ratio
          + global_payment_txs_ratio + global_new_order_ratio;
  if (sum != 100) {
      printf("==== ERROR: the sum of the ratios of tx types does not match 100: %d\n", sum);
      exit(1);
  }
  if (global_num_warehouses > num_warehouses) {
      printf("==== ERROR: the number of warehouses is too large\n");
      exit(1);
  }

  //TM_STARTUP(num_clients);

  // TM_THREAD_EXIT();
  // P_MEMORY_SHUTDOWN();
  // GOTO_SIM();
  // thread_shutdown();
  //TM_SHUTDOWN();

	// SIM_GET_NUM_CPU(num_clients);
	// TM_STARTUP(num_clients);
	// P_MEMORY_STARTUP(num_clients);
	// thread_startup(num_clients);
  

  uint64_t total_shared = get_shared_memory_used();
  uint64_t total_priv = get_local_memory_used();  //should be zero
  printf("------------------------------------------------------------------\n");
  printf("Total persistent memory used at this point:\n");
  printf("   shared : %lu\n", total_shared);
  printf("   private: %lu\n", total_priv);
  printf("   total  : %lu\n", total_shared+total_priv);
  printf("------------------------------------------------------------------\n");


  
  #ifdef USE_PAGING
    struct paging_init_var Paging;     
    Paging.HEAP_START_ADDR = getNumaBaseAddress();      
    // Paging.memory_heap_mmap = 1024 * 1024 * 1024;
    Paging.memory_heap_size = (SIZE_OF_PRIV * num_clients + SIZE_OF_SHARED / 2);       
    Paging.memory_heap_mmap = heap_mmap;//Paging.memory_heap_size; //
    
//    printf ("heap_mmap: %ld, heap_size: %ld\n", heap_mmap, Paging.memory_heap_size);
    if ((uint64_t)(SIZE_OF_PRIV * num_clients + SIZE_OF_SHARED / 2) < (uint64_t)heap_mmap){
      printf("Total memory is smaller than the heap area. Aborting ...\n");
      exit(-1);
    }
    // uint64_t sizeMmap = SIZE_OF_PRIV;    
    // Paging.memory_heap_mmap = sizeMmap; /*default*/
    // printf ("paging after mmap = %lu\n", Paging.memory_heap_mmap);
    Paging.addPageThreshold = 0.97; 
    Paging.rmPageThreshold = .95;
    paging_init(Paging);
  #endif

//  printf("Running... \n");
//  fflush(stdout);
  GOTO_SIM();
  
  // warmup phase
  printf("Warmup phase ... \n");
  begin = local_clock->getMicroseconds();
  thread_start(warmup, clients);
  end = local_clock->getMicroseconds();
  printf("Warmup phase over ... %g secs\n", (end-begin)/1000000.0);

  paging_reset_stats();

  printf("Running... \n");
  fflush(stdout);
  begin = local_clock->getMicroseconds();
  thread_start(client, clients);

  end = local_clock->getMicroseconds();
  GOTO_REAL();
  int64_t microseconds = end - begin;

  unsigned long executed_stock_level_txs = 0;
  unsigned long executed_delivery_txs = 0;
  unsigned long executed_order_status_txs = 0;
  unsigned long executed_payment_txs = 0;
  unsigned long executed_new_order_txs = 0;

  for (c = 0; c < num_clients; c++) {
    executed_stock_level_txs += clients[c]->executed_stock_level_txs_;
    executed_delivery_txs += clients[c]->executed_delivery_txs_;
    executed_order_status_txs += clients[c]->executed_order_status_txs_;
    executed_payment_txs += clients[c]->executed_payment_txs_;
    executed_new_order_txs += clients[c]->executed_new_order_txs_;
  }

  double sum_txs_exec = executed_stock_level_txs + executed_delivery_txs + executed_order_status_txs
          + executed_payment_txs + executed_new_order_txs;
  printf("------------------------------------------------------------------\n");
  printf("\nExecuted the following txs types:\n");
  printf("\tStockLevel : %.2f\t%lu\n", (executed_stock_level_txs / sum_txs_exec), executed_stock_level_txs);
  printf("\tDelivery   : %.2f\t%lu\n", (executed_delivery_txs / sum_txs_exec), executed_delivery_txs);
  printf("\tOrderStatus: %.2f\t%lu\n", (executed_order_status_txs / sum_txs_exec), executed_order_status_txs);
  printf("\tPayment    : %.2f\t%lu\n", (executed_payment_txs / sum_txs_exec), executed_payment_txs);
  printf("\tNewOrder   : %.2f\t%lu\n", (executed_new_order_txs / sum_txs_exec), executed_new_order_txs);

  printf("%ld transactions in %ld ms = %.2f txns/s\n", (long)sum_txs_exec,
          (microseconds + 500)/1000, sum_txs_exec / (double) microseconds * 1000000.0);

  // printf("Txs: %ld\n", (long)sum_txs_exec);
  // #ifndef USE_TXS
  stats_benchTime = microseconds / 1000000.0;
  printf("Total time (secs): %.3f\n", stats_benchTime);
  // #endif
  // printf("------------------------------------------------------------------\n");
  
  total_shared = get_shared_memory_used();
  printf("------------------------------------------------------------------\n");
  printf("Total persistent memory used at the end:\n");
  printf("   shared : %ld\n", total_shared);
  int ii=0;
  total_priv = 0;
  for (ii=0; ii<num_clients; ii++) {
    uint64_t prv_mem = clients[ii]->total_pm_consumed_;
    printf("   private (t%d): %ld\n",  ii, prv_mem);
    total_priv += prv_mem;
  }
  printf("   total  : %ld\n", total_shared+total_priv);
  printf("------------------------------------------------------------------\n");

  P_MEMORY_SHUTDOWN();
  thread_shutdown();
  TM_SHUTDOWN();

  #ifdef USE_PAGING
    paging_finish();
  #endif

  return 0;
}
