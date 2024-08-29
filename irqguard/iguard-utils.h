#ifndef iguard_H
#define iguard_H

#include <linux/module.h>
#include <linux/types.h>


#define DEBUGCTL                  0x1D9
#define PERF_GLOBAL_STATUS        0x38E
#define PERF_GLOBAL_CTRL          0x38F
#define PERF_GLOBAL_STATUS_RESET  0x390
#define FIXED_CTR0                0x309
#define FIXED_CTR_CTRL            0x38D
#define PMC0                      0xC1
#define PERFEVTSEL0               0x186

extern int kPmcsCount;
extern int kFixedPmcsCount;
extern int kPmcMaxWidth;
extern int kFixedPmcMaxWidth;
extern int kSupportedEventMask;


// execute function with argument pointer 'info' on guarded CPU
void execute_on_guarded_cpu(smp_call_func_t func, void* info);

// writes MSR on guarded CPU
void msr_write(int unsigned msr_addr, uint64_t value);

// reads MSR on guarded CPU
uint64_t msr_read(int unsigned msr_addr);

// stops performance counter
void disable_perf_globally(void);

// starts peformance counters
void enable_perf_globally(void);

// configure PMC freeze on PMI
void perf_globally_freeze_on_PMI(void);

// configure PMC to not freeze on PMI
void perf_globally_run_on_PMI(void);

// reset status after overflow happened
// (has to be called to allow another overflow)
void reset_overflow_status(void);

// reset status after freeze happened
// (has to be called to allow couting again)
void reset_freeze_status(void);

// write 0 to all PMCs
void clear_perf_value_globally(void);

// write 0 to all PERFEVTSELs
void clear_perf_config(void);

// configure specified PMC
void configure_pmc(int pmc_number, int event_selector, int umask);

void enable_and_configure_pmc_all(void);

void enable_fixed_ctr_all(void);

int check_cpuid(void);

#endif










