#include "iguard-utils.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/version.h>


#include "iguard-common.h"

//
// Globals and Constants
//

int kPmcsCount;
int kFixedPmcsCount;
int kPmcMaxWidth;
int kFixedPmcMaxWidth;
int kSupportedEventMask;

//
//
//

bool is_guarded_cpu_core(int cpu, void* info) {
  return cpu == GUARDED_CPU_CORE;
}

void execute_on_guarded_cpu(smp_call_func_t func, void* info) {
  // TODO: didn't check when exactly the API changed so the boundary could be off
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
  on_each_cpu_cond(is_guarded_cpu_core, func, info, true);
#else
  on_each_cpu_cond(is_guarded_cpu_core, func, info, true, 0);
#endif
}

struct msr_write_request {
	uint64_t msr_addr;
	uint64_t value;
};

void inline msr_write_inner(void* info) {
	struct msr_write_request* request = (struct msr_write_request*) info;
	long unsigned lower = (long unsigned) request->value;
	long unsigned upper = (long unsigned) (request->value >> 32);
	asm ("wrmsr" : : "a" (lower), "d" (upper), "c" (request->msr_addr));
}

void msr_write(int unsigned msr_addr, uint64_t value) {
	struct msr_write_request write_request = {.msr_addr = msr_addr, .value = value};
	execute_on_guarded_cpu(msr_write_inner, (void*) &write_request);
}

struct msr_read_request {
	uint64_t msr_addr;
	uint64_t value;
};

void msr_read_inner(void* info) {
	struct msr_read_request* request = (struct msr_read_request*) info;
	long unsigned lower, upper;
	asm ("rdmsr" : "=a" (lower), "=d" (upper) : "c" (request->msr_addr));
	request->value = (uint64_t) upper << 32 | lower;
}

uint64_t msr_read(int unsigned msr_addr) {
	struct msr_read_request read_request = {.msr_addr = msr_addr, .value = -1};
	execute_on_guarded_cpu(msr_read_inner, (void*) &read_request);
	return read_request.value;
}

void msr_set_nth_bit(uint64_t msr, size_t bit_pos) {
  uint64_t val = msr_read(msr);
  val |= 1 << bit_pos;
  msr_write(msr, val);
}

void msr_unset_nth_bit(uint64_t msr, size_t bit_pos) {
  uint64_t val = msr_read(msr);
  val &= ~(1 << bit_pos);
  msr_write(msr, val);
}

void disable_perf_globally() {
	msr_write(PERF_GLOBAL_CTRL, 0);
}

void enable_perf_globally() {
	long unsigned lower = (1 << kPmcsCount) - 1;
	long unsigned upper = 7;
	msr_write(PERF_GLOBAL_CTRL, (uint64_t) upper << 32 | lower);
}

void perf_globally_freeze_on_PMI() {
	uint64_t val = msr_read(DEBUGCTL);
	// set IA32_DEBUGCTL.Freeze_PerfMon_On_PMI (bit 12)
	val |= ((uint64_t)1 << 12);
	msr_write(DEBUGCTL, val);
}

void perf_globally_run_on_PMI() {
	uint64_t val = msr_read(DEBUGCTL);
	val &= ~((uint64_t) 1 << 12);
	msr_write(DEBUGCTL, val);
}

void reset_overflow_status() {
	// Intel Manual Table 17-3 
	// (Legacy and Streamlined Operation with Freeze_perfmon_On_PMI=1, Counter Overflowed)

	uint64_t reset_value = msr_read(PERF_GLOBAL_STATUS);
	printk(KERN_WARNING "iguard: RESET VALUE -> %llx\n", reset_value);
	msr_write(PERF_GLOBAL_STATUS_RESET, reset_value);

	// old version
	//long unsigned lower = (1 << kPmcsCount) - 1;
	//msr_write(PERF_GLOBAL_STATUS_RESET, lower);
  msr_unset_nth_bit(DEBUGCTL, 11);
}

void reset_freeze_status() {
	msr_write(PERF_GLOBAL_STATUS_RESET, 1ULL << 59);

	msr_write(PERF_GLOBAL_STATUS_RESET, 3ULL);
}

void clear_perf_config() {
  int i;
  for (i = 0; i < kPmcsCount; i++) {
		msr_write(PERFEVTSEL0+i, 0);
  }

  msr_write(FIXED_CTR_CTRL, 0);
}

void clear_perf_value_globally() {
	msr_write(FIXED_CTR0, 0);
	msr_write(FIXED_CTR0+1, 0);
	msr_write(FIXED_CTR0+2, 0);
	
  int i;
	for (i = 0; i < kPmcsCount; i++) {
		msr_write(PMC0+i, 0);
	}
}

void configure_pmc(int pmc_number, int event_selector, int umask) {
	if (pmc_number >= kPmcsCount) {
		printk(KERN_ERR "iguard: Requested PMC number (%d) does not exist (max: %d)\n", pmc_number, kPmcsCount - 1);
	}

	uint64_t perfevtselx = 0;
	perfevtselx =  0 << 24; // Counter Mask

	perfevtselx |= 0 << 23; // Invert Counter Mask
	perfevtselx |= 1 << 22; // Enable Counters
	perfevtselx |= 0 << 21; // Any Thread (also count sibling thread events)
	perfevtselx |= 1 << 20; // APIC interrupt enable
	perfevtselx |= 0 << 19; // Pin control

	perfevtselx |= 0 << 18; // Edge detect

	perfevtselx |= 0 << 17; // OS flag
	perfevtselx |= 1 << 16; // USR flag
	perfevtselx |= umask << 8;     // UMASK
	perfevtselx |= event_selector; // Event Selector
	msr_write(PERFEVTSEL0 + pmc_number, perfevtselx);

	printk(KERN_INFO "iguard: Configured PMC%d on CPU %d\n", 
         pmc_number, 
         GUARDED_CPU_CORE);
}

void enable_and_configure_pmc_all() {
	uint64_t perfevtselx;

	if(kPmcsCount >= 1 && (kSupportedEventMask & 0x40) == 0) {
		//Branch misses retired is available!
		perfevtselx = (0 << 24); 		// Counter Mask
		perfevtselx |= (0 << 23);		// Invert Counter Mask
		perfevtselx |= (1 << 22);		// Enable Counters
		perfevtselx |= (1 << 21);		// Any Thread (also count HyperThread events)
		perfevtselx |= (1 << 20);		// APIC interrupt enable
		perfevtselx |= (0 << 19);		// Pin control
		perfevtselx |= (0 << 18);		// Edge detect
		perfevtselx |= (0 << 17);		// OS flag
		perfevtselx |= (1 << 16);		// USR flag
		perfevtselx |= (0 << 8);		// UMASK
		perfevtselx |= 0xC5;				// Event Selector
		msr_write(PERFEVTSEL0, perfevtselx);
	}
	if(kPmcsCount >= 2 && (kSupportedEventMask & 0x10) == 0) {
		//LLC misses is available!
		perfevtselx = (0 << 24); 		// Counter Mask
		perfevtselx |= (0 << 23);		// Invert Counter Mask
		perfevtselx |= (1 << 22);		// Enable Counter
		perfevtselx |= (0 << 21);		// Nothing
		perfevtselx |= (1 << 20);		// Interrupts
		perfevtselx |= (0 << 19);		// Pin control
		perfevtselx |= (0 << 18);		// Edge detect
		perfevtselx |= (0 << 17);		// OS flag
		perfevtselx |= (1 << 16);		// USR flag
		perfevtselx |= (0x41 << 8);	// UMASK
		perfevtselx |= 0x2E;				// Event Selector
		msr_write(PERFEVTSEL0+1, perfevtselx);
	}
}

//CTR0 = Retired Instructions, CTR1 = core cycles not in halt, CTR2 = core cycles not in halt but as TSC, CTR3 = Topdown slots
void enable_fixed_ctr_all() {
	msr_write(FIXED_CTR_CTRL, 0xAAA);	//A is 1010, which is: PMI on, only count in user level! Use B for user and kernel count
}

// taken from Linux' cpuid.h to resolve include problems on newer kernels
#define cpuid(level, a, b, c, d)					\
  __asm__ __volatile__ ("cpuid\n\t"					\
			: "=a" (a), "=b" (b), "=c" (c), "=d" (d)	\
			: "0" (level))

int check_cpuid() {
	unsigned int eax, ebx, ecx, edx;
	int perf_monitoring_version;
	char proc_vendor_string[17] = {0};


	cpuid(0, eax, ebx, ecx, edx);
	memcpy(proc_vendor_string, (char*)&ebx, 4);
	memcpy(proc_vendor_string+4, (char*)&edx, 4);
	memcpy(proc_vendor_string+8, (char*)&ecx, 4);

	if (strcmp(proc_vendor_string, "GenuineIntel") != 0) {
		// TODO: currently we focus on Intel only
		return 1;
	}

	//Check whether MSRs are allowed
	cpuid(0x01, eax, ebx, ecx, edx);
	if (!(edx & (1 << 5))) {
		printk(KERN_ERR "iguard: No MSRs supported! edx: %d\n", edx);
		return 1;
	}

	cpuid(0x0A, eax, ebx, ecx, edx);
	perf_monitoring_version = (eax & 0xFF);
	printk(KERN_INFO "iguard: Performance monitoring version: %d\n", perf_monitoring_version);
	if (perf_monitoring_version < 4) {
		printk(KERN_ERR "iguard: Error: performance monitoring version == 4 required\n");
		return 1;
	}
	
	kPmcsCount = ((eax >> 8) & 0xFF);
	printk(KERN_INFO "iguard: Number of general-purpose performance counters: %u\n", kPmcsCount);
	
	kPmcMaxWidth = (eax & 0xFF0000) >> 16;
	printk(KERN_INFO "iguard: PMC max width: %u\n", kPmcMaxWidth);
	
	kSupportedEventMask = ebx & 0xFF;
	printk(KERN_INFO "iguard: PMC event selection mask: %u\n", kSupportedEventMask);

  kFixedPmcsCount = edx & 0x1F;
	printk(KERN_INFO "iguard: Number of fixed-function performance counters: %u\n", kFixedPmcsCount);
	kFixedPmcMaxWidth = (edx & 0x1FE0) >> 5;
	printk(KERN_INFO "iguard: Fixed CTR max width: %u\n", kFixedPmcMaxWidth);
	 
	return 0;
}
