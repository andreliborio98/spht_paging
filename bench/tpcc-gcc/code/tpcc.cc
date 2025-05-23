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

#define DEFAULT_STOCK_LEVEL_TXS_RATIO       4
#define DEFAULT_DELIVERY_TXS_RATIO          4
#define DEFAULT_ORDER_STATUS_TXS_RATIO      4
#define DEFAULT_PAYMENT_TXS_RATIO           43
#define DEFAULT_NEW_ORDER_TXS_RATIO         45
#define DEFAULT_NUMBER_WAREHOUSES           1
#define DEFAULT_TIME_SECONDS                10
#define DEFAULT_NUM_CLIENTS					1

int duration_secs;

#include "../backends/gcc-abi/libitm.h"

void* client(void *data) {
    _ITM_initializeThread();
    TPCCClient* client = (TPCCClient*) data;
    SystemClock* clock = new SystemClock();
    int64_t begin = clock->getMicroseconds();

    do {
        client->doOne();
    } while (((clock->getMicroseconds() - begin) / 1000000) < duration_secs);
}

int main(int argc, char** argv) {

    if (argc < 9) {
        printf("Please provide all the minimum parameters\n");
        exit(1);
    }

    struct option long_options[] = {
      // These options don't set a flag
      {"stockLevel transactions ratio",     required_argument, NULL, 's'},
      {"delivery transactions ratio",       required_argument, NULL, 'd'},
      {"order status transactions ratio",   required_argument, NULL, 'o'},
      {"payment transactions ratio",        required_argument, NULL, 'p'},
      {"new order txs ratio",               required_argument, NULL, 'r'},
      {"number warehouses",                 required_argument, NULL, 'w'},
      {"duration in seconds",               required_argument, NULL, 't'},
      {"number of clients",		            required_argument, NULL, 'n'},
      {"workload change",                   required_argument, NULL, 'c'},
      {"maximum number of warehouses",      required_argument, NULL, 'm'},
      {NULL, 0, NULL, 0}
    };

    global_stock_level_txs_ratio = DEFAULT_STOCK_LEVEL_TXS_RATIO;
    global_delivery_txs_ratio = DEFAULT_DELIVERY_TXS_RATIO;
    global_order_status_txs_ratio = DEFAULT_ORDER_STATUS_TXS_RATIO;
    global_payment_txs_ratio = DEFAULT_PAYMENT_TXS_RATIO;
    global_new_order_ratio = DEFAULT_NEW_ORDER_TXS_RATIO;
    int num_warehouses = DEFAULT_NUMBER_WAREHOUSES;
    duration_secs = DEFAULT_TIME_SECONDS;
    int num_clients = DEFAULT_NUM_CLIENTS;

    // If "-c" is found, then we start parsing the parameters into the workload_changes.
    // The argument of each "-c" is the number of seconds that it lasts.
    std::vector<int> workload_changes;
    int adapt_workload = 0;

    int i, c;
    while(1) {
        i = 0;
        c = getopt_long(argc, argv, "s:d:o:p:r:w:t:n:c:m:", long_options, &i);
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
             workload_changes.push_back(atoi(optarg));
           break;
         case 'd':
             workload_changes.push_back(atoi(optarg));
           break;
         case 'o':
             workload_changes.push_back(atoi(optarg));
           break;
         case 'p':
             workload_changes.push_back(atoi(optarg));
           break;
         case 'r':
             workload_changes.push_back(atoi(optarg));
           break;
         case 'w':
             workload_changes.push_back(atoi(optarg));
           break;
         case 'm':
             num_warehouses = atoi(optarg);
             break;
         case 't':
             duration_secs = atoi(optarg);
           break;
         case 'n':
        	 num_clients = atoi(optarg);
        	 break;
         default:
           printf("Incorrect argument! :(\n");
           exit(1);
        }
      }

    TPCCTables* tables = new TPCCTables();
    SystemClock* clock = new SystemClock();

    // Create a generator for filling the database.
    tpcc::RealRandomGenerator* random = new tpcc::RealRandomGenerator();
    tpcc::NURandC cLoad = tpcc::NURandC::makeRandom(random);
    random->setC(cLoad);

    // Generate the data
    printf("Loading %ld warehouses... ", num_warehouses);
    fflush(stdout);
    char now[Clock::DATETIME_SIZE+1];
    clock->getDateTimestamp(now);
    TPCCGenerator generator(random, now, Item::NUM_ITEMS, District::NUM_PER_WAREHOUSE,
            Customer::NUM_PER_DISTRICT, NewOrder::INITIAL_NUM_PER_DISTRICT);
    int64_t begin = clock->getMicroseconds();
    generator.makeItemsTable(tables);
    for (int i = 0; i < num_warehouses; ++i) {
        generator.makeWarehouse(tables, i+1);
    }
    int64_t end = clock->getMicroseconds();
    printf("%ld ms\n", (end - begin + 500)/1000);

    // Change the constants for run
    random = new tpcc::RealRandomGenerator();
    random->setC(tpcc::NURandC::makeRandomForRun(random, cLoad));

    // Client owns all the parameters
    TPCCClient** clients = (TPCCClient**) malloc(num_clients * sizeof(TPCCClient*));
    pthread_t* threads = (pthread_t*) malloc(num_clients * sizeof(pthread_t));
    for (c = 0; c < num_clients; c++) {
        clients[c] = new TPCCClient(clock, random, tables, Item::NUM_ITEMS, static_cast<int>(num_warehouses),
                District::NUM_PER_WAREHOUSE, Customer::NUM_PER_DISTRICT);
    }

    int64_t next_workload_secs;
    uint32_t pos_vec = 0;
    if (adapt_workload) {
        next_workload_secs = workload_changes[pos_vec++];
    } else {
        next_workload_secs = duration_secs;
    }

    global_num_warehouses = workload_changes[pos_vec++];
    global_stock_level_txs_ratio = workload_changes[pos_vec++];
    global_delivery_txs_ratio = workload_changes[pos_vec++];
    global_order_status_txs_ratio = workload_changes[pos_vec++];
    global_payment_txs_ratio = workload_changes[pos_vec++];
    global_new_order_ratio = workload_changes[pos_vec++];

    printf("Running with the following parameters for %ld secs: (max warehouses %d)\n", next_workload_secs, num_warehouses);
    printf("\tWarehouses     (-w): %d\n", global_num_warehouses);
    printf("\tStockLevel ratio   (-s): %d\n", global_stock_level_txs_ratio);
    printf("\tDelivery ratio     (-d): %d\n", global_delivery_txs_ratio);
    printf("\tOrder Status ratio (-o): %d\n", global_order_status_txs_ratio);
    printf("\tPayment ratio      (-p): %d\n", global_payment_txs_ratio);
    printf("\tNewOrder ratio     (-r): %d\n", global_new_order_ratio);

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

    printf("Running... ");
    fflush(stdout);
    begin = clock->getMicroseconds();
    for (c = 0; c < num_clients; c++) {
    	pthread_create(&threads[c], NULL, client, clients[c]);
    }

    do {
        usleep(next_workload_secs * 1000 * 1000);
        if (pos_vec >= workload_changes.size()) {
            break;
        }

        next_workload_secs = workload_changes[pos_vec++];
        global_num_warehouses = workload_changes[pos_vec++];
        global_stock_level_txs_ratio = workload_changes[pos_vec++];
        global_delivery_txs_ratio = workload_changes[pos_vec++];
        global_order_status_txs_ratio = workload_changes[pos_vec++];
        global_payment_txs_ratio = workload_changes[pos_vec++];
        global_new_order_ratio = workload_changes[pos_vec++];

        printf("Running with the following parameters for %ld secs: (max warehouses %d)\n", next_workload_secs, num_warehouses);
        printf("\tWarehouses     (-w): %d\n", global_num_warehouses);
        printf("\tStockLevel ratio   (-s): %d\n", global_stock_level_txs_ratio);
        printf("\tDelivery ratio     (-d): %d\n", global_delivery_txs_ratio);
        printf("\tOrder Status ratio (-o): %d\n", global_order_status_txs_ratio);
        printf("\tPayment ratio      (-p): %d\n", global_payment_txs_ratio);
        printf("\tNewOrder ratio     (-r): %d\n", global_new_order_ratio);
    } while (true);

    for (c = 0; c < num_clients; c++) {
    	pthread_join(threads[c], NULL);
    }

    end = clock->getMicroseconds();
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
    printf("\nExecuted the following txs types:\n");
    printf("\tStockLevel : %.2f\t%lu\n", (executed_stock_level_txs / sum_txs_exec), executed_stock_level_txs);
    printf("\tDelivery   : %.2f\t%lu\n", (executed_delivery_txs / sum_txs_exec), executed_delivery_txs);
    printf("\tOrderStatus: %.2f\t%lu\n", (executed_order_status_txs / sum_txs_exec), executed_order_status_txs);
    printf("\tPayment    : %.2f\t%lu\n", (executed_payment_txs / sum_txs_exec), executed_payment_txs);
    printf("\tNewOrder   : %.2f\t%lu\n", (executed_new_order_txs / sum_txs_exec), executed_new_order_txs);

    printf("%ld transactions in %ld ms = %.2f txns/s\n", (long)sum_txs_exec,
            (microseconds + 500)/1000, sum_txs_exec / (double) microseconds * 1000000.0);

    printf("Txs: %ld\n", (long)sum_txs_exec);
    printf("Total time (secs): %.3f\n", (microseconds / 1000000.0));

    return 0;
}
