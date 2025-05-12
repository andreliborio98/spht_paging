#include "eigenbench.h"
#include "threading.h"
#include "timer.h"

#include <vector>
#include "tm_impl.h" // TODO: implement the TM

using namespace std;

enum EB_ACTION {
  EB_READ  = 1,
  EB_WRITE = 2
};


/* extern */double eb_run_duration;
static eb_params_s eb_params;

thread_local static vector<long> history_Array1;
thread_local static vector<long> history_Array2;

static void init_arrays();
static void destroy_arrays();

static EB_ACTION random_action(
  eb_params_s &p, long **array, long &r1, long &w1, long &r2, long &w2
);
static long random_index(
  eb_params_s &p, long *array
);
static long local_ops(
  eb_params_s &p, long R3, long W3, long Nops, long &val
);

static void callback(int id, int nb_thrs, void *arg);

static void eb_init(eb_params_s params)
{
  eb_params = params;
  init_arrays();
}

static void eb_destroy()
{
  destroy_arrays();
}

void eb_run(eb_params_s params)
{
  EB_TM_INIT(params.N);
  eb_init(params);
  
  TIMER_T start, stop;
  threading_start(params.N, 0, callback, NULL);
  // TODO: take times
  TIMER_READ(start);
  threading_join();
  TIMER_READ(stop);
  eb_run_duration = TIMER_DIFF_SECONDS(start, stop);

  EB_TM_DESTROY();
  eb_destroy();
}

static void eb_test_core(eb_params_s p)
{
  long val = 0;
  long total = p.W1 + p.W2 + p.R1 + p.R2;
  long(*local_ops_ptr)(eb_params_s&,long,long,long,long&) = local_ops;
  TIMER_T time1, time2;

  p.Array3 = (long*)EB_MALLOC(sizeof(long)*p.A3);
  for (int i = 0; i < sizeof(long)*p.A3; i += 4096 / sizeof(long)) {
    val += p.Array3[i]; // triggers CoW
  }

  TIMER_READ(time1);
  TIMER_READ(time2);
  int i = 0;
  while (TIMER_DIFF_SECONDS(time1, time2) * 1000.0f < p.loops) {
    long r1 = p.R1;
    long r2 = p.R2;
    long w1 = p.W1;
    long w2 = p.W2;
    long *array;
    EB_ACTION action;
    long index;
    uint64_t seed = p.seed;

    history_Array1.clear(); // TODO: HTM transactions do not like this
    history_Array2.clear();

    history_Array1.reserve(1024);
    history_Array2.reserve(1024);

    EB_TM_BEGIN();

    if (p.persist) p.seed = seed; // for STM rollbacks

    if (p.k_i == 1 && total == 0) {
      // only local operations
      val += local_ops_ptr(p, p.R3_i, p.W3_i, p.Nop_i, val);
    } else {
      // some non-local operations
      for (int j = 0; j < total; ++j) {
        action = random_action(p, &array, r1, w1, r2, w2);
        index = random_index(p, array);
        if (action == EB_READ) {
          val += EB_TM_READ(&(array[index]));
        } else { // action == EB_WRITE
          EB_TM_WRITE(&(array[index]), val);
        }
        if (p.k_i == 1 || (p.k_i != 0 && (j % p.k_i) == 0)) {
          val += local_ops_ptr(p, p.R3_i, p.W3_i, p.Nop_i, val);
        }
      }
    }

    EB_TM_END();

    if (p.k_o == 1 || (p.k_o != 0 && (i % p.k_o) == 0)) {
      val += local_ops_ptr(p, p.R3_o, p.W3_o, p.Nop_o, val);
    }
    i++;
    TIMER_READ(time2);
  }
  EB_FREE(p.Array3);
}

// --- internal

static void init_arrays()
{
  eb_params.Array1 = (long*)EB_MALLOC(sizeof(long)*eb_params.A1);
  eb_params.Array2 = (long*)EB_MALLOC(sizeof(long)*eb_params.A2*eb_params.N);
  // eb_params.Array3 = (long*)EB_MALLOC(sizeof(long)*eb_params.A3*eb_params.N); // local per thread
}

static void destroy_arrays()
{
  EB_FREE(eb_params.Array1);
  EB_FREE(eb_params.Array2);
  // EB_FREE(eb_params.Array3);
}

static EB_ACTION random_action(
  eb_params_s &p, long **array, long &r1, long &w1, long &r2, long &w2
) {
  // options: 0 -> (Read  Array1)
  //          1 -> (Write Array1)
  //          2 -> (Read  Array2)
  //          3 -> (Write Array2)
  uint64_t option = EB_RAND_R_FNC(p.seed) % 4;
  EB_ACTION ret = EB_READ;
  *array = nullptr;

  switch (option) {
    case 0:
      ret = EB_READ;
      *array = p.Array1;
      break;
    case 1:
      ret = EB_WRITE;
      *array = p.Array1;
      break;
    case 2:
      ret = EB_READ;
      *array = p.Array2;
      break;
    case 3:
      ret = EB_WRITE;
      *array = p.Array2;
      break;
  }
  return ret;
}

static long random_index(
  eb_params_s &p, long *array
) {
  // options: 0 -> read history
  //          1 -> (Write Array1)

  int isRepeated = (EB_RAND_R_FNC(p.seed) & 0xFFFF) > (p.lct * (float)0x10000);
  long index = 0;

  if (array == p.Array1) {

    if (isRepeated && !history_Array1.empty()) {
      index = EB_RAND_R_FNC(p.seed) % history_Array1.size();
      index = history_Array1[index];
    } else {
      index = EB_RAND_R_FNC(p.seed) % p.A1;
      history_Array1.push_back(index);
    }

  } else { // array == p.Array2

    if (isRepeated && !history_Array2.empty()) {
      index = EB_RAND_R_FNC(p.seed) % history_Array2.size();
      index = history_Array2[index];
    } else {
      index = (EB_RAND_R_FNC(p.seed) % p.A2) + (p.A2 * p.tid);
      history_Array2.push_back(index);
    }

  }
  return index;
}

static long local_ops(
  eb_params_s &p, long R3, long W3, long Nops, long &val
) {
  // options: 0 -> read history
  //          1 -> (Write Array1)

  long r3 = R3, w3 = W3;
  long total = R3 + W3;

  for (int i = 0; i < total; ++i) {
    int isWrite = (EB_RAND_R_FNC(p.seed) % 2) && w3 > 0;
    long index = (EB_RAND_R_FNC(p.seed) % p.A3) + (p.A3 * p.tid);

    if (isWrite) {
      w3--;
      p.Array3[index] = val;
    } else { // isRead
      r3--;
      val = p.Array3[index];
    }
  }

  for (int i = 0; i < Nops; ++i) {
    EB_NOP_INST;
  }

  return val;
}

static void callback(int id, int nb_thrs, void *arg)
{
  eb_params_s p = eb_params; // do not optimize!
  p.tid = id;
  EB_THREAD_ENTER(id);
  eb_test_core(p);
  EB_THREAD_EXIT(id);
}
