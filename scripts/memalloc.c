#include <stdio.h>
#include <stdlib.h> 
#include <stdint.h>
#include <sys/time.h>

// #define memory_heap_size 262144
// #define memory_heap_mmap 131027 //196563 //65536
// #define page_size        4096
// #define qtt_accesses	 10000

static __thread unsigned long RND_FN_seed = 1234;

#define RAND_R_FNC(seed) ({ \
  unsigned long next = seed; \
  unsigned long result; \
  next *= 1103515245; \
  next += 12345; \
  result = (unsigned long) (next / 65536) % 2048; \
  next *= 1103515245; \
  next += 12345; \
  result <<= 10; \
  result ^= (unsigned long) (next / 65536) % 1024; \
  next *= 1103515245; \
  next += 12345; \
  result <<= 10; \
  result ^= (unsigned long) (next / 65536) % 1024; \
  seed = next; \
  result; \
})

void page_out(unsigned long mem_array_flag[], int memory_heap_size, int page_size){
	unsigned long index = RAND_R_FNC(RND_FN_seed) % (memory_heap_size/page_size);
	int count = 0;
	while (0 == mem_array_flag[index] && count < (memory_heap_size/page_size)){
		index = (index+1) % (memory_heap_size/page_size);
		count++;
	}
	if (mem_array_flag[index] != 0){
		mem_array_flag[index] = 0;
	}
}

int main(int argc, char *argv[]){
 	int memory_heap_size = atoi(argv[1]);
 	int memory_heap_mmap = atoi(argv[2]);
 	int page_size = atoi(argv[3]);
 	int qtt_accesses = atoi(argv[4]);
 	int percent_page_fault = atoi(argv[5]);

	int qtt_mem_pages = memory_heap_size / page_size;
	unsigned long mem_array[1000], *mem_array_flag, i, k, random, index;
	struct timeval start, end;
	double t, sum;
	int mem_mapped = memory_heap_mmap;
	int mem_unmapped = memory_heap_size - memory_heap_mmap;
	int max_pages_mapped_mem = mem_mapped / page_size;
	int max_pages_unmapped_mem = qtt_mem_pages - max_pages_mapped_mem;
	uintptr_t rnd;

  mem_array_flag = (unsigned long *)malloc(qtt_mem_pages*sizeof(unsigned long));


	FILE * fp;

	if (percent_page_fault > 100){
		printf ("Input Error");
		exit(1);
	}

	for (i=0; i<qtt_mem_pages; i++){
		mem_array_flag[i] = -1;
		if ((i >= max_pages_mapped_mem) && (i < max_pages_mapped_mem + max_pages_unmapped_mem)){
			mem_array_flag[i] = 0;
		}
		if (i < max_pages_mapped_mem){
			mem_array_flag[i] = 1;
		}
	}
	for (i=0; i<1000; i++){
		mem_array[i] = -1;
	}

  int only_one_pf = 0;

	gettimeofday(&start, NULL);
	fp = fopen ("./../nvhtm/bench/mem_addr_array.c", "w");
	fprintf(fp, "long mem_addr_array[] = {");
	int access_counter = 0;
  unsigned long nb_pf = 0;

	for (i=0; i <= qtt_accesses; i++){
		if (i % 1000 == 0 && i!=0){
			// printf("in if i %d\n", i);
			for (int j=0; j < 1000; j++){
				if (j>0){
					fprintf(fp, ", %lu", mem_array[j]);
					if (j%10==0)
						fprintf(fp, "\n");
				}else{
					fprintf(fp, "%lu", mem_array[j]);
					if (j%10==0 && j!=0)	
						fprintf(fp, "\n");
				}
			}
			if (i != qtt_accesses){
				fprintf(fp, ", ");
			}
			gettimeofday(&end, NULL);
			t = (double) ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)) / 1000000.0;
			//printf ("%lu, checkmark --> %fs \n", i, t);
			gettimeofday(&start, NULL);
			if (i == qtt_accesses)
				break;
			access_counter = 0;
		}
		// printf("ac %d\n", access_counter);

		rnd = rand() % 100;

		k=0;
		if (rnd >= percent_page_fault || (only_one_pf)){
			index = rand() % qtt_mem_pages;
			while (k < qtt_mem_pages){
				if (mem_array_flag[index]==1){
					random = rand() % 512;
					mem_array[access_counter] = (index*page_size/8) + random; 
					break;
				}
				k++;
				index = (index+1) % (qtt_mem_pages);
			}
		} else {
      ///only_one_pf = 1;
      nb_pf++;
			index = rand() % qtt_mem_pages;
			while (k < qtt_mem_pages){
				if (mem_array_flag[index] == 0){
					random = rand() % 512;
					mem_array[access_counter] = (index*page_size/8) + random;
					page_out(mem_array_flag, memory_heap_size, page_size);
					mem_array_flag[index] = 1;
					break;
				}
				k++;
				index = (index+1) % (qtt_mem_pages);
			}
		}
		access_counter++;
	}
  printf("Total pf: %lu\n", nb_pf);
	fprintf(fp, "};");
	fclose(fp);
}
