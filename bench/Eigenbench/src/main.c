#include "eigenbench.h"
#include "input_handler.h"
#include "timer.h"

#include <string.h>
#include <stdio.h>

static eb_params_s params;

static int parse_arg(const char *arg);
static void check_params();

int main(int argc, char **argv)
{
  input_parse(argc, argv);

  if (input_exists("--help") || input_exists("-h")) {
    printf("EigenBench exploration tool for orthogonal TM characteristics\n");
    printf(" Usage: %s [parameter=<value>]*\n", argv[0]);
    printf("   N=<int>       Number of threads\n");
    printf("   loops=<int>   Number of milliseconds doing TXs\n");
    printf("   persist=<0|1> Repeat random seed\n");
    printf("   lct=<float>   Prob. repeat same addresses\n");
    printf("   R1=<int>      Reads/TX in Hot array\n");
    printf("   W1=<int>      Writes/TX in Hot array\n");
    printf("   R2=<int>      Reads/TX in Mild array\n");
    printf("   W2=<int>      Writes/TX in Mild array\n");
    printf("   R3_i=<int>    Reads/loop in Cold array inside TX\n");
    printf("   W3_i=<int>    Writes/loop in Cold array inside TX\n");
    printf("   R3_o=<int>    Reads/loop in Cold array outside TX\n");
    printf("   W3_o=<int>    Writes/loop in Cold array outside TX\n");
    printf("   k_i=<int>     Scaler for in-TX local ops\n");
    printf("   k_o=<int>     Scaler for out-TX local ops\n");
    printf("   Nop_i=<int>   In-TX no-ops\n");
    printf("   Nop_o=<int>   Out-TX no-ops\n");
    printf("   A1=<int>      Size of Array 1 (Hot array)\n");
    printf("   A2=<int>      Size of Array 2 (Mild array)\n");
    printf("   A3=<int>      Size of Array 3 (Cold array)\n");
    printf("   seed=<int>    Random seed\n");
    return EXIT_SUCCESS;
  }

  printf(" --- INPUT PARAMETERS --- \n");
  input_foreach(parse_arg);
  printf(" --- CHECK PARAMETERS --- \n");
  check_params();
  printf(" --- ---------------- --- \n");

  eb_run(params);

  printf(" --- RESULTS -------- --- \n");
  printf("Time: %f ms\n", eb_run_duration * 1000.0f);

  return EXIT_SUCCESS;
}

static int parse_arg(const char *arg)
{
  printf("Got input %s ", arg);
  fflush(stdout);
  if (strcmp(arg, "N") == 0) {
    params.N = input_getLong("N");
    printf(" <params.N> = %li", params.N);
  }
  if (strcmp(arg, "loops") == 0) {
    params.loops = input_getLong("loops");
    printf(" <params.loops> = %li", params.loops);
  }
  if (strcmp(arg, "persist") == 0) {
    params.persist = input_getLong("persist");
    printf(" <params.persist> = %i", params.persist);
  }
  if (strcmp(arg, "lct") == 0) {
    params.lct = input_getDouble("lct");
    printf(" <params.lct> = %f", params.lct);
  }
  if (strcmp(arg, "R1") == 0) {
    params.R1 = input_getLong("R1");
    printf(" <params.R1> = %li", params.R1);
  }
  if (strcmp(arg, "W1") == 0) {
    params.W1 = input_getLong("W1");
    printf(" <params.W1> = %li", params.W1);
  }
  if (strcmp(arg, "R2") == 0) {
    params.R2 = input_getLong("R2");
    printf(" <params.R2> = %li", params.R2);
  }
  if (strcmp(arg, "W2") == 0) {
    params.W2 = input_getLong("W2");
    printf(" <params.W2> = %li", params.W2);
  }
  if (strcmp(arg, "R3_o") == 0) {
    params.R3_o = input_getLong("R3_o");
    printf(" <params.R3_o> = %li", params.R3_o);
  }
  if (strcmp(arg, "W3_o") == 0) {
    params.W3_o = input_getLong("W3_o");
    printf(" <params.W3_o> = %li", params.W3_o);
  }
  if (strcmp(arg, "R3_i") == 0) {
    params.R3_i = input_getLong("R3_i");
    printf(" <params.R3_i> = %li", params.R3_i);
  }
  if (strcmp(arg, "W3_i") == 0) {
    params.W3_i = input_getLong("W3_i");
    printf(" <params.W3_i> = %li", params.W3_i);
  }
  if (strcmp(arg, "k_i") == 0) {
    params.k_i = input_getLong("k_i");
    printf(" <params.k_i> = %li", params.k_i);
  }
  if (strcmp(arg, "k_o") == 0) {
    params.k_o = input_getLong("k_o");
    printf(" <params.k_o> = %li", params.k_o);
  }
  if (strcmp(arg, "Nop_i") == 0) {
    params.Nop_i = input_getLong("Nop_i");
    printf(" <params.Nop_i> = %li", params.Nop_i);
  }
  if (strcmp(arg, "Nop_o") == 0) {
    params.Nop_o = input_getLong("Nop_o");
    printf(" <params.Nop_o> = %li", params.Nop_o);
  }
  if (strcmp(arg, "A1") == 0) {
    params.A1 = input_getLong("A1");
    printf(" <params.A1> = %li", params.A1);
  }
  if (strcmp(arg, "A2") == 0) {
    params.A2 = input_getLong("A2");
    printf(" <params.A2> = %li", params.A2);
  }
  if (strcmp(arg, "A3") == 0) {
    params.A3 = input_getLong("A3");
    printf(" <params.A3> = %li", params.A3);
  }
  if (strcmp(arg, "seed") == 0) {
    params.seed = input_getLong("seed");
    printf(" <params.seed> = %llu", params.seed);
  }
  printf("\n");
  return 0;
}

static void check_params()
{
  printf(" params.N       = %li\n", params.N);
  printf(" params.loops   = %li\n", params.loops);
  printf(" params.persist = %i\n", params.persist);
  printf(" params.lct     = %f\n", params.lct);
  printf(" params.R1      = %li\n", params.R1);
  printf(" params.W1      = %li\n", params.W1);
  printf(" params.R2      = %li\n", params.R2);
  printf(" params.W2      = %li\n", params.W2);
  printf(" params.R3_i    = %li\n", params.R3_i);
  printf(" params.W3_i    = %li\n", params.W3_i);
  printf(" params.R3_o    = %li\n", params.R3_o);
  printf(" params.W3_o    = %li\n", params.W3_o);
  printf(" params.k_i     = %li\n", params.k_i);
  printf(" params.k_o     = %li\n", params.k_o);
  printf(" params.Nop_i   = %li\n", params.Nop_i);
  printf(" params.Nop_o   = %li\n", params.Nop_o);
  printf(" params.A1      = %li\n", params.A1);
  printf(" params.A2      = %li\n", params.A2);
  printf(" params.A3      = %li\n", params.A3);
  printf(" params.seed    = %llu\n", params.seed);
}
