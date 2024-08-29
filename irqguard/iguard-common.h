#ifndef iguard_COMMON_H
#define iguard_COMMON_H


//
// ATTENTION: this file must be valid in both user and kernel mode
//

#ifdef __KERNEL__
#include <linux/kernel.h>
#else
#include <stdint.h>
#endif

// name of the /proc file to communicate
#define PROC_API_FNAME "iguard"
#define API_DEV_NAME "iguard"

// CPU core that is used for iguard
// TODO: breaks if CORE is isolated
#define GUARDED_CPU_CORE 0

#define READY_EVENT 0x1337

// Maximum number of PMCs
#define MAX_PMCS 8

// Maximum number of Fixed Function Counters
#define MAX_FIXEDCOUNTERS 4

// commands for the user library kernel module
//enum ApiCommands {
//  kStartProfiling = 0,
//  kStopProfiling = 1,
//  kStartGuarding = 2,
//  kStopGuarding = 3,
//  kUpdateConfig = 4,
//};

struct iguard_request_args {
  int pmc_number;
  int event_selector;
  int umask;
  int threshold;
};

struct iguard_pmc_results {
  uint64_t pmc[MAX_PMCS];
  uint64_t fixed_counters[MAX_FIXEDCOUNTERS];
};

//
// IOCTL Command Definitions
//
#define iguard_IOCTL_MAGIC_NUM (long) 0xf00f001337

// MAGIC_NUM + Command-ID + sizeof ioctl return bytes
#define CMD_START_PROFILING _IOR(iguard_IOCTL_MAGIC_NUM, 1, size_t)
#define CMD_STOP_PROFILING  _IOR(iguard_IOCTL_MAGIC_NUM, 2, size_t)
#define CMD_START_GUARDING  _IOR(iguard_IOCTL_MAGIC_NUM, 3, size_t)
#define CMD_STOP_GUARDING   _IOR(iguard_IOCTL_MAGIC_NUM, 4, size_t)
#define CMD_UPDATE_CONFIG   _IOR(iguard_IOCTL_MAGIC_NUM, 5, size_t)

#endif /* !iguard_COMMON_H */
