// required for iguard
#define _GNU_SOURCE


#include <string.h>
#include "iguard-api.h"

void signal_handler(int signal) {
	char msg[] = "\033[93mSignal SIGUSR1\033[0m\n";
	write(1, msg, sizeof(msg));
}


void prompt_key() {
	printf("Press key to continue\n");
	//getchar();
}

static void flush(void *p) {
  asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax");
}

static inline void maccess(void *p) {
  asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}

void mfence() { asm volatile("mfence"); }


__attribute__((aligned(4096))) char mem1[4096];
__attribute__((aligned(4096))) char mem2[4096];
__attribute__((aligned(4096))) char mem3[4096];
__attribute__((aligned(4096))) char mem4[4096];
__attribute__((aligned(4096))) char mem5[4096];

int main(int argc, char** argv) {
	int c = 0;
	memset(mem1, 0x31, 4096);
	memset(mem2, 0x31, 4096);
	memset(mem3, 0x31, 4096);
	memset(mem4, 0x31, 4096);
	memset(mem5, 0x31, 4096);
	
	iguard_init();
		
	printf("Updating config...\n");
	prompt_key();

	
	// -> moved to file
	//iguard_set_pmc_config(0, 0x2e, 0x41, PERF_COUNT_HW_CACHE_MISSES, 0x40);  // LONGEST_LAT_CACHE.MISS (lab17/coffelake)
	iguard_parse_pmc_config("./pmc_config.iguard");

	//printf("Profiling...\n");
	//prompt_key();
	
	flush(mem1);
	mfence();

	printf("Test phase next\n");
	prompt_key();

	flush(mem1);
	flush(mem2);
	flush(mem3);
	mfence();
	iguard_start_protection();
	//iguard_start_profiling();
	if (iguard_no_attack()) {
		maccess(mem1);
		maccess(mem2);
		maccess(mem3);
		mfence();
	} else {
		printf("\033[93m[!!] Attack detected (that should not happen!)\033[0m\n");
	}
	iguard_stop_protection();
	//iguard_stop_profiling();
	
	printf("Test shootdown next\n");
	prompt_key();

	flush(mem1);
	flush(mem2);
	flush(mem3);
	mfence();



	printf("before guarding\n");
	size_t i = 0;
	printf("=== Triggering Attack 1 ===\n");
	iguard_start_protection();
	//printf("after guarding\n");
	if (iguard_no_attack()) {
		for (; i < 1000; i++) {
			flush(mem1);
			flush(mem2);
			flush(mem3);
			flush(mem4);
			flush(mem5);
			mfence();
			maccess(mem1);
			maccess(mem2);
			maccess(mem3);
			maccess(mem4);
			maccess(mem5);
			mfence();
		}
		printf("\033[91m=== you should not see this print!!! ===\033[0m\n");
	} else {
		printf("\033[92m[!!] Attack detected (that should happen!)\033[0m\n");
	}
	printf("loop iterations: %zu\n", i);
	printf("after loop\n");
	iguard_stop_protection();

	printf("=== Triggering Attack 2 ===\n");
	i = 0;
	iguard_start_protection();

	if (iguard_no_attack()) {
		for (; i < 1000; i++) {
			flush(mem1);
			flush(mem2);
			flush(mem3);
			flush(mem4);
			flush(mem5);
			mfence();
			maccess(mem1);
			maccess(mem2);
			maccess(mem3);
			maccess(mem4);
			maccess(mem5);
			mfence();
		}
		printf("\033[91m=== you should not see this print!!! ===\033[0m\n");
	} else {
		printf("\033[92m[!!] Attack detected (that should happen!)\033[0m\n");
	}
	iguard_stop_protection();

	printf("Finished\n");
	prompt_key();
	iguard_cleanup();

	return 0;
}
