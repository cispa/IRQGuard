// required for iguard
#define _GNU_SOURCE


#include <string.h>
#include "iguard-benchmark-api.h"  // makefile generates it by setting -D BENCHMARK on the API


//#define MEASURE_RETIRED_ONLY


#define INTELASM(code) ".intel_syntax noprefix\n\t" code "\n\t.att_syntax prefix\n"

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


void benchmark() {
	size_t i = 0;
	iguard_start_guarding();
	//printf("after guarding\n");
	if (iguard_guarding()) {
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
			//mfence();
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			asm volatile(INTELASM("divps xmm4, xmm5"));
			//maccess(mem4);
			//maccess(mem5);
			//mfence();
		}
		printf("\033[91m=== you should not see this print!!! ===\033[0m\n");
	} else {
		printf("\033[92m[!!] Attack detected (that should happen!)\033[0m\n");
	}
	printf("loop iterations: %zu\n", i);
	printf("after loop\n");
	iguard_stop_guarding();

}

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

	
	iguard_set_pmc_config(0, 0x2e, 0x41, PERF_COUNT_HW_CACHE_MISSES, 0x5);  // LONGEST_LAT_CACHE.MISS (lab17/coffelake)

	// we write 0 as threshold to just get the "normal" PMC without any modifications
	iguard_set_pmc_config(2, 0xc0, 0x00, PERF_COUNT_HW_INSTRUCTIONS, 0x0);  // Instructions Retired

 #ifdef MEASURE_RETIRED_ONLY
	// measures retired instructions
	iguard_set_pmc_config(3, 0xc7, 0x08, PERF_COUNT_HW_INSTRUCTIONS, 0x0);  // FP_ARITH_INST_RETIRED.128B_PACKED_SINGLE (divps should work with that)
 #else
	// measures executed cycles
	iguard_set_pmc_config(3, 0x14, 0x01, PERF_COUNT_HW_INSTRUCTIONS, 0x0);  // ARITH.DIVIDER_ACTIVE (coffeelake/lab17)
#endif
	//printf("Profiling...\n");
	//prompt_key();
	
	flush(mem1);
	mfence();

	printf("Test phase next\n");
	prompt_key();

	maccess(mem1);
	mfence();
	iguard_start_guarding();
	if (iguard_guarding()) {
		maccess(mem1);
		maccess(mem1);
		maccess(mem1);
		mfence();
	} else {
		printf("\033[93m[!!] Attack detected (that should not happen!)\033[0m\n");
	}
	iguard_stop_guarding();
	
	printf("Test shootdown next\n");
	prompt_key();

	benchmark();
	benchmark();

	printf("======DELIMITER========\n");
	benchmark();



	printf("Finished\n");
	iguard_cleanup();

	return 0;
}
