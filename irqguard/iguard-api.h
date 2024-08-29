#ifndef PMCMONITOR_H
#define PMCMONITOR_H
#define _GNU_SOURCE

#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>
#include <locale.h>
#include <limits.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <sched.h>
#include <setjmp.h>
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>

#include "iguard-common.h"

// 
// Configuration
// 

// Uncomment to switch to the userspace version of iguard
//#define USERSPACE

// Uncomment to switch to benchmarking mode (required for benchmark_retired_instructions.c)
//#define BENCHMARK

// TODO: PEBS-related code could be removed if the switch to pintool-based prefetching is commited 

//
// Globals and Constants
//

#define iguard

static int kApiFd = -1;
static int kUserspaceFd = -1;

int kJmpBufferSet = 0;
static jmp_buf kJmpBuffer;
#define iguard_no_attack() (!setjmp(kJmpBuffer) && (kJmpBufferSet = 1))

// TODO: prefetching it not implemented for userspace variant currently
#ifndef USERSPACE
#include <sys/mman.h>

#define HOT_SECTION_BEGIN_MARKER_ADDR 0x31337000
#define HOT_SECTION_END_MARKER_ADDR 0x31338000
static void* kHotSectionBeginMarker;
static void* kHotSectionEndMarker;
#define FNAME_MAXLEN 255
static char kPrefetchConfigFilename[FNAME_MAXLEN];
// contains list of addresses to prefetch
static uint64_t* kPrefetchConfig;
static size_t kPrefetchConfigLength;
#define MEMTRACING_ACTIVE_FILE "/tmp/memtracing_active.iguard"
#define PROFILING_ACTIVE_FILE "/tmp/profiling_active.iguard"
static int kMemtraceGenerationIsActive;
static int kProfilingIsActive;
#endif

//
//
//

static inline void iguard_inner__maccess(void *p) {
	asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}

static inline void iguard_inner__maccessw(void *p) {
  asm volatile("movq %%rax, (%0)\n" : : "c"(p) : "rax");
}

long iguard_inner__perf_event_open(struct perf_event_attr* event_attr, pid_t pid,
																 int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, event_attr, pid, cpu, group_fd, flags);
}

void iguard_inner__set_cpu_affinity(int cpu) {
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  int s = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
  if (s != 0) {
    fprintf(stderr, "[-] Could not set cpu affinity (core %d)!\n", cpu);
    exit(-1);
  }
}


void iguard_inner__event_handler(int signum, siginfo_t* info, void* ucontext) {
#ifdef BENCHMARK
	// we use PMC2 for benchmarking the num of intructions retired
	uint64_t low, high;
	asm volatile("rdpmc" :"=a"(low), "=d"(high) : "c"(0x3));
	printf("Div Active Cycles: %zu\n", low);
#endif

	if (signum != SIGUSR1) {
		printf("\033[91m[!!] Caught unexpected signal!\033[0m\n");
		exit(1);
	}

	if (!kJmpBufferSet) {
		printf("\033[91m[!!] Caught signal before JmpBuffer was set!\033[0m\n");
		exit(1);
	}
#ifdef USERSPACE
	// reset iguard state
  ioctl(info->si_fd, PERF_EVENT_IOC_REFRESH, 1);
#endif

	// unmask signal
	sigset_t signal_set;
	sigemptyset(&signal_set);
	sigaddset(&signal_set, SIGUSR1);
	sigprocmask(SIG_UNBLOCK, &signal_set, NULL);

  longjmp(kJmpBuffer, 1);
}

static void iguard_inner__initialize_prefetch_config_name() {
	//snprintf(kPrefetchConfigFilename, FNAME_MAXLEN,
	//			  "%s_prefetch_config.iguard", program_invocation_short_name);
	// TODO: do we need this dynamic?
	snprintf(kPrefetchConfigFilename, FNAME_MAXLEN,
				  "./prefetch_config.iguard");
}

static void iguard_inner__initialize_prefetching() {
	iguard_inner__initialize_prefetch_config_name();

	if (access(kPrefetchConfigFilename,R_OK) != 0) {
		// config not existant. nothing to do.
		printf("\033[93miguard: No prefetch config given. Prefetching will not happen.\033[0m\n");
		return;
	}

	FILE* fd = fopen(kPrefetchConfigFilename, "r");
	if (fd == 0) {
    fprintf(stderr, "iguard: Failed to open prefetch-config-file\n");
		exit(1);
	}
	ssize_t bytes_read;
	char* line = NULL;
	size_t len = 0;
	size_t config_buffer_allocated_entries = 10;
	kPrefetchConfig = malloc(config_buffer_allocated_entries * sizeof(uint64_t));
	size_t line_no = 0;
	while ((bytes_read = getline(&line, &len, fd)) != -1) {
		// read line from file
		uint64_t addr = 0;
		if (line[0] == '#' || iguard_inner__str_is_whitespace(line)) {
			// ignore empty lines and those that start with #
			continue;
		}
		addr = strtoull(line, NULL, 16);
		if (addr == ULLONG_MAX) {
			char cwd[4096];
			realpath(kPrefetchConfigFilename, cwd);
			fprintf(stderr, "FPATH: %s, line: %s\n", cwd, line);
			exit(1);

		}

		// resize buffer
		if (kPrefetchConfigLength == config_buffer_allocated_entries) {
			config_buffer_allocated_entries *= 2;
			kPrefetchConfig = realloc(kPrefetchConfig, 
													config_buffer_allocated_entries * sizeof(uint64_t));
		}

		// write to buffer
		kPrefetchConfig[kPrefetchConfigLength++] = addr;

	}
	fclose(fd);
}

int iguard_inner__check_profiling_state() {
	// check whether we run in profiling mode
	if (access(PROFILING_ACTIVE_FILE, F_OK) == 0) {
		// file exists signaling us that profiling is active on the monitored program.
		// this effectively replaces the start/stop_guarding calls with start/stop_profiling
		printf("\033[93miguard: Profiling mode is active. Guarding calls are replaced by profiling calls.\033[0m\n");
		kProfilingIsActive = 1;
	}
	return kProfilingIsActive;
}

int iguard_inner__check_memtracing_state() {
	// check whether we run in memory tracing mode
	if (access(MEMTRACING_ACTIVE_FILE, F_OK) == 0) {
		// file exists signaling us that PEBS is active on the monitored program. thus we disable
		// all calls to the API as the kmodule should not be loaded right now and we mark the 
		// memory accesses for the "hot section"
		printf("\033[93miguard: MemTracing mode is active. No API calls will be made.\033[0m\n");
		kMemtraceGenerationIsActive = 1;
	}
	return kMemtraceGenerationIsActive;
}

void iguard_init(void) {
#ifdef BENCHMARK
	// enable userspace rdpmc
	system("echo 2 | sudo tee /sys/devices/cpu/rdpmc");
#endif
	// enables comma-prints for numbers in printf
	setlocale(LC_NUMERIC, "");

	char fname[255] = {0};
	snprintf(fname, 255, "/dev/%s", PROC_API_FNAME);
	
	// Register signal handler
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = iguard_inner__event_handler;
  sa.sa_flags = SA_SIGINFO;
	int signal = SIGUSR1;
  if (sigaction(signal, &sa, NULL) < 0) {
      fprintf(stderr, "iguard: Failed to setup signal handler\n");
			exit(1);
  }

	// pin program to guarded core
	iguard_inner__set_cpu_affinity(GUARDED_CPU_CORE);
	
	// check whether we run in profiling mode
	iguard_inner__check_profiling_state();
	// check whether we run in memtracing mode

#ifndef USERSPACE
	iguard_inner__initialize_prefetching();
	if (iguard_inner__check_memtracing_state()) {
		// if we do, we do not need to create the ApiFd
		return;
	}
	kApiFd = open(fname, O_RDONLY);
	if (kApiFd < 0) {
		fprintf(stderr, "[-] Could not open %s\n", fname);
		exit(1);
	}

#endif
}

void iguard_set_pmc_config(size_t pmc_number,
                         uint8_t event_selector,
												 uint8_t umask,
												 int perf_event,
												 uint64_t threshold) {
	if (kMemtraceGenerationIsActive) {
		// do nothing if PEBS is active as there will be no module loaded
		return;
	}
	if (pmc_number >= MAX_PMCS) {
		fprintf(stderr, "[-] Specified PMC config index (%zu) out of bounds!"
		" Aborting!\n", pmc_number);
		exit(1);
	}

//#ifndef BENCHMARK
//	if (threshold <= 0) {
//		fprintf(stderr, "[-] Threshold must be a positive value > 0\n");
//		exit(1);
//	}
//#endif

#ifndef USERSPACE
	struct iguard_request_args request_args = {
		.pmc_number = pmc_number,
		.event_selector = event_selector,
		.umask = umask,
		.threshold = threshold
	};

	int ret = ioctl(kApiFd, CMD_UPDATE_CONFIG, (unsigned long)&request_args);
	assert(ret == 0);


#else
  struct perf_event_attr pe;
  memset(&pe, 0, sizeof(struct perf_event_attr));
  pe.type = PERF_TYPE_HARDWARE;
  pe.size = sizeof(struct perf_event_attr);
  pe.config = perf_event;
  pe.disabled = 1;
  pe.sample_type = PERF_SAMPLE_IP;
  pe.sample_period = threshold;
  pe.exclude_kernel = 1;
  pe.exclude_hv = 1;
  
  pid_t pid = 0;
  int cpu = -1;
  int group_fd = -1;
  unsigned long flags = 0;

  int fd = iguard_inner__perf_event_open(&pe, pid, cpu, group_fd, flags);
  if (fd == -1) {
      printf("[-] iguard: perf_event_open failed, did you run as root?\n");
			exit(1);
  }

    // event handler for overflows
    fcntl(fd, F_SETFL, O_NONBLOCK|O_ASYNC);
    fcntl(fd, F_SETSIG, SIGUSR1);
    fcntl(fd, F_SETOWN, getpid());
    
    kUserspaceFd = fd;
#endif
}


void iguard_set_default_config_thresholds(uint64_t threshold_pmc0,
																				uint64_t threshold_pmc1,
																				uint64_t threshold_pmc2,
																				uint64_t threshold_pmc3,
																				uint64_t threshold_pmc4,
																				uint64_t threshold_pmc5,
																				uint64_t threshold_pmc6,
																				uint64_t threshold_pmc7) {
	if (kMemtraceGenerationIsActive) {
		// do nothing if PEBS is active as there will be no module loaded
		return;
	}
	// LLC Misses (A)
	iguard_set_pmc_config(0, 0x2e, 0x41, -1, threshold_pmc0);
	// Branch Misses Retired (A)
	iguard_set_pmc_config(1, 0xC5, 0x00, -1, threshold_pmc1);

	// MACHINE_CLEARS.COUNT (skylake; lab12)
	iguard_set_pmc_config(2, 0xc3, 0x01, -1, threshold_pmc2);
	// DTLB_LOAD_MISSES.WALK_STLB_HIT (skylake; lab12)
	iguard_set_pmc_config(3, 0x08, 0x20, -1, threshold_pmc3);
	//iguard_set_pmc_config(3, 0x2e, 0x41, -1, threshold_pmc3);
	//iguard_set_pmc_config(4, 0x2e, 0x41, -1, threshold_pmc4);
	//iguard_set_pmc_config(5, 0x2e, 0x41, -1, threshold_pmc5);
	//iguard_set_pmc_config(6, 0x2e, 0x41, -1, threshold_pmc6);
	//iguard_set_pmc_config(7, 0x2e, 0x41, -1, threshold_pmc7);
}

int iguard_inner__str_is_whitespace(const char* str) {
  while (*str) {
    if (!isspace(*str))
      return 0;
    str++;
  }
  return 1;
}

void iguard_parse_pmc_config(const char* config_fname) {
	FILE* fd = fopen(config_fname, "r");
	if (fd == 0) {
		printf("config_fname: %s\n", config_fname);
		fprintf(stderr, "iguard: Could not open PMC config file. Aborting!\n");
		exit(1);
	}

	ssize_t bytes_read;
	char* line = NULL;
	size_t len = 0;
	size_t line_no = 0;
	while ((bytes_read = getline(&line, &len, fd)) != -1) {
		line_no++;
		// format: pmc-number;event-sel;umask;threshold
		size_t pmc_number;
		uint8_t event_sel;
		uint8_t umask;
		uint64_t threshold;
		if (line[0] == '#' || iguard_inner__str_is_whitespace(line)) {
			// ignore empty lines and those that start with #
			continue;
		}
		int parsed = sscanf(line, "%lli;%hhi;%hhi;%lli", &pmc_number, &event_sel, &umask, &threshold);

		if (parsed != 4) {
			fprintf(stderr, "iguard: Error in PMC config file (line: %zu -> err %d)."
							" Aborting!\n", line_no, parsed);
			exit(1);
		}
		iguard_set_pmc_config(pmc_number, event_sel, umask, -1, threshold);
	}
	fclose(fd);
}

static inline void iguard_inner__prefetch(void *p) {
  asm volatile("prefetcht0 0(%0)\n" : : "c"(p) : "eax");
}

static inline void iguard_inner__mfence() {
	asm volatile("mfence");
}

static void iguard_inner__prefetch_if_required() {
	if (kPrefetchConfig == NULL) {
		// if there is no config we do not prefetch
		return;
	}
	// prefetch entries, one by one
	for (size_t i = 0; i < kPrefetchConfigLength; i++) {
		iguard_inner__prefetch(kPrefetchConfig[i]);
		// TODO: accessing does not work for stack accesses atm (see obsidian todo)
		//iguard_inner__maccess(kPrefetchConfig[i]);
	}
	iguard_inner__mfence();
}

static inline void iguard_inner__verr() {
	// instruction does nothing besides enabling/disabling the memory trace recording of pintool
	int ds = 0;
  asm volatile("verr %[ds]" : : [ds] "m" (ds) : "cc");
}

static void iguard_inner__mark_hot_section_begin() {
	iguard_inner__verr();
}

static void iguard_inner__mark_hot_section_end() {
	iguard_inner__verr();
}

static void iguard_inner__start_profiling() {
#ifndef USERSPACE
	if (kMemtraceGenerationIsActive) {
		// with PEBS active, we do nothing besides marking the hot section via memory accesses
		iguard_inner__mark_hot_section_begin();
		return;
	}
	iguard_inner__prefetch_if_required();
	// blocking call
	int ret = ioctl(kApiFd, CMD_START_PROFILING, NULL);
	assert(ret == 0);
#else
	assert(0);
#endif
}

static void iguard_inner__stop_profiling() {
#ifndef USERSPACE
	if (kMemtraceGenerationIsActive) {
		// with PEBS active, we do nothing besides marking the hot section via memory accesses
		iguard_inner__mark_hot_section_end();
		return;
	}
	struct iguard_pmc_results pmc_results = {0};
	int ret = ioctl(kApiFd, CMD_STOP_PROFILING, (unsigned long) &pmc_results);
	assert(ret == 0);

 	//
	// print results
	//
	printf("=========== Profiling Results ==============\n");
	for (size_t i = 0 ; i < MAX_FIXEDCOUNTERS; i++) {
		printf("  FF%zu:\t\t%'zu\n", i, pmc_results.fixed_counters[i]);
	}
	for (size_t i = 0 ; i < MAX_PMCS; i++) {
		printf("  PMC%zu:\t\t%'zu\n", i, pmc_results.pmc[i]);
	}
	printf("============================================\n");

#else
	assert(0);
#endif
}

static void iguard_inner__start_guarding() {
#ifndef USERSPACE
	if (kMemtraceGenerationIsActive) {
		// with PEBS active, we do nothing besides marking the hot section via memory accesses
		iguard_inner__mark_hot_section_begin();
		return;
	}
	iguard_inner__prefetch_if_required();
	// blocking call
	int ret = ioctl(kApiFd, CMD_START_GUARDING, NULL);
	assert(ret == 0);
#else
	assert(kUserspaceFd > 0);
  ioctl(kUserspaceFd, PERF_EVENT_IOC_RESET, 0);
  ioctl(kUserspaceFd, PERF_EVENT_IOC_REFRESH, 1);
#endif
}

static void iguard_inner__stop_guarding() {
#ifndef USERSPACE
	if (kMemtraceGenerationIsActive) {
		// with PEBS active, we do nothing besides marking the hot section via memory accesses
		iguard_inner__mark_hot_section_end();
		return;
	}
	int ret = ioctl(kApiFd, CMD_STOP_GUARDING, NULL);
	assert(ret == 0);

#else
	assert(kUserspaceFd > 0);
  ioctl(kUserspaceFd, PERF_EVENT_IOC_DISABLE, 0);
  //long long counter;
  //read(kUserspaceFd, &counter, sizeof(long long));
	//printf("counter: %lld\n", counter);
#endif
}

static void iguard_start_protection() {
	if (kProfilingIsActive) {
		iguard_inner__start_profiling();
	} else {
		iguard_inner__start_guarding();
	}
}

static void iguard_stop_protection() {
	if (kProfilingIsActive) {
		iguard_inner__stop_profiling();
	} else {
		iguard_inner__stop_guarding();
	}
}

void iguard_cleanup() {
#ifdef BENCHMARK
	system("echo 1 | sudo tee /sys/devices/cpu/rdpmc");
#endif

#ifndef USERSPACE
	close(kApiFd);
	if (kPrefetchConfig) {
		free(kPrefetchConfig);
		kPrefetchConfig = NULL;
	}

#else
    close(kUserspaceFd);
#endif
}

#endif  // PMCMONITOR_H
