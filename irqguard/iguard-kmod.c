#include <asm/apic.h>
#include <asm/nmi.h>
#include <asm/perf_event.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/sched.h>

#include "iguard-common.h"
#include "iguard-utils.h"

#ifndef READBUF_SIZE
#define READBUF_SIZE 255
#endif


MODULE_LICENSE("GPL");

//
// Globals and Constants
//

int kActiveUid;  // Linux does not assign negative values
struct task_struct* kActiveTask;
struct pmc_measurement kCurrentProfile;
int kProfilingActive = 0;
int kGuardingActive = 0;
int kNmiHandlerCalled = 0;
int kPmcThresholds[MAX_PMCS] = {0};

//
//

//
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#define from_user raw_copy_from_user
#define to_user raw_copy_to_user
#else
#define from_user copy_from_user
#define to_user copy_to_user
#endif


struct pmc_measurement {
  unsigned count;
  unsigned long long fixed0;
  unsigned long long fixed1;
  unsigned long long pmc0;
  unsigned long long pmc1;
};

static void print_current_measurement(void) {
  printk(KERN_INFO
         "Measurement Averages: (Count: %d)\n"
         "fixed0: %llu\n"
         "fixed1: %llu\n"
         "pmc0: %llu\n"
         "pmc1: %llu\n",
         kCurrentProfile.count, kCurrentProfile.fixed0,
         kCurrentProfile.fixed1, kCurrentProfile.pmc0,
         kCurrentProfile.pmc1);
}

void check_freeze_status(void) {
  uint64_t val = msr_read(PERF_GLOBAL_STATUS);
  if (val & (1ULL << 59)) {
    printk(KERN_WARNING
           "iguard: === Warning ===\n"
           "-> msrs were still frozen! Triggering reset\n");
    reset_freeze_status();
  }
}

static int pmi_event_handler(unsigned int cmd, struct pt_regs* regs) {

  uint64_t perf_global_status = msr_read(PERF_GLOBAL_STATUS);
  // this check requires that we are registered with NMI_FLAG_FIRST, 
  // i.e., that we execute before other NMI handlers that reset PERF_GLOBAL_STATUS again
  if (perf_global_status == 0) {
    // TODO: probably disable this prevent again to prevent performance issues
    printk(KERN_INFO "iguard: Woken up by unrelated NMI\n");
    // no counter overflowed -> we should not handle anything

    // unmask the NMI handler (required for newer kernels)
    apic_write(APIC_LVTPC, APIC_DM_NMI);

    // NMI_DONE -> we did all that we could but the kernel should check 
    //             whether other handlers can do better
    return NMI_DONE;
  }

  if (kGuardingActive) {
    // signal userspace process that the guarding
    // detected malicious activity 
    printk(KERN_INFO "iguard: Sending SIGNAL\n");
    send_sig(SIGUSR1, kActiveTask, 0);

    // enables us to have a debug prints and prevents sending multiple signals
    kNmiHandlerCalled = 1;
  }
  
  kGuardingActive = 0;

  // log PMC results to visualize which counter overflowed
  // TODO: can we do that inside a PMI handler? prob not

  int fixed_ctr_idx = 0;
  for (fixed_ctr_idx = 0; fixed_ctr_idx < kFixedPmcsCount; fixed_ctr_idx++) {
    uint64_t pmc_val = msr_read(FIXED_CTR0 + fixed_ctr_idx);
    printk(KERN_WARNING "iguard: Guarding result (FF%d) -> %d\n", fixed_ctr_idx, pmc_val);
  }
  int pmc_idx = 0;
  for (pmc_idx = 0; pmc_idx < kPmcsCount; pmc_idx++) {
    uint64_t pmc_val = msr_read(PMC0 + pmc_idx);
    printk(KERN_WARNING "iguard: Guarding result (PMC%d) -> %d\n", pmc_idx, pmc_val);
  }

  disable_perf_globally();
  // reset_perf_status_globally();
  reset_overflow_status();
  reset_freeze_status();
  perf_globally_freeze_on_PMI();

  // unmask the NMI handler (required for newer kernels)
  apic_write(APIC_LVTPC, APIC_DM_NMI);
  return NMI_HANDLED;
}

static void init_pmc_configurations(void) {
  disable_perf_globally();
  clear_perf_value_globally();
  clear_perf_config();
  reset_overflow_status();
  reset_freeze_status();
  //enable_and_configure_pmc_all();
  //enable_fixed_ctr_all();
  perf_globally_freeze_on_PMI();

  printk(KERN_INFO "iguard: Initialized PMC configuration\n");
}

static void start_profiling(void) {
  // TODO: prevent attackers from abusing this check
  if (kProfilingActive || kGuardingActive) {
    printk(KERN_WARNING "PCMD: Start was called twice! Inproper API usage!\n");
    return;
  }
  kActiveUid = current_uid().val;
  kActiveTask = current;

  printk(KERN_INFO "iguard: Starting profiling on behalf of UID %d with PID %d (%s)\n",
      kActiveUid, kActiveTask->pid, kActiveTask->comm);

  // Reset everything to 0 and enable counters
  kProfilingActive = 1;
  clear_perf_value_globally();
  enable_perf_globally();
}

static void stop_profiling(struct iguard_pmc_results* userspace_pmc_results) {
  // Disable counters, and store results
  disable_perf_globally();

  // copy struct from userspace
  struct iguard_pmc_results pmc_results;
  from_user(&pmc_results, (void*) userspace_pmc_results, sizeof(struct iguard_pmc_results));

  if (kProfilingActive && kActiveTask->pid == current->pid) {
    // Allowed access
    kCurrentProfile.count = kCurrentProfile.count + 1;


    int fixed_ctr_idx = 0;
    for (fixed_ctr_idx = 0; fixed_ctr_idx < kFixedPmcsCount; fixed_ctr_idx++) {
      uint64_t pmc_val = msr_read(FIXED_CTR0 + fixed_ctr_idx);
      kCurrentProfile.fixed0 =
          (kCurrentProfile.fixed0 * (kCurrentProfile.count - 1) + pmc_val) /
          kCurrentProfile.count;
      printk(KERN_WARNING "iguard: Profile result (FF%d) -> %d\n", fixed_ctr_idx, pmc_val);
      pmc_results.fixed_counters[fixed_ctr_idx] = pmc_val;
    }

    int pmc_idx = 0;
    for (pmc_idx = 0; pmc_idx < kPmcsCount; pmc_idx++) {
      uint64_t pmc_val = msr_read(PMC0 + pmc_idx);
      kCurrentProfile.pmc0 =
          (kCurrentProfile.pmc0 * (kCurrentProfile.count - 1) + pmc_val) /
          kCurrentProfile.count;
      printk(KERN_WARNING "iguard: Profile result (PMC%d) -> %d\n", pmc_idx, pmc_val);
      pmc_results.pmc[pmc_idx] = pmc_val;
    }

    to_user((void*) userspace_pmc_results, &pmc_results, sizeof(struct iguard_pmc_results));

    printk(KERN_INFO
        "iguard: Stopped profiling on behalf of UID %d with PID %d (%s)\n",
        current_uid().val, current->pid, current->comm);
    //print_current_measurement();

    kProfilingActive = 0;

  } else {
    enable_perf_globally();
  }
}

static void start_guarding(void) {
  // TODO: we also need to check here that the UID
  //  is a trusted process or smth like that
  if (kGuardingActive) {
    printk(KERN_ERR "iguard: Nested Guarding mode?? Likely incorrect API usage!\n");
    //return;
  }

  check_freeze_status();
  //perf_globally_freeze_on_PMI();
  printk(KERN_INFO "iguard: DEBUGCTL -> %lx\n", msr_read(DEBUGCTL));

  kActiveUid = current_uid().val;
  kActiveTask = current;

  clear_perf_value_globally();

  // TODO: enable fixedfunction counters again if needed
  //uint64_t threshold = kCurrentProfile.fixed0 + 10 * 2;
  //uint64_t pmc_value = (1UL << kFixedPmcMaxWidth) -  threshold;
  //msr_write(FIXED_CTR0, pmc_value);

  //threshold = kCurrentProfile.fixed1 + 10 * 2;
  //pmc_value = (1UL << kFixedPmcMaxWidth) - threshold;
  //msr_write(FIXED_CTR0 + 1, pmc_value);


  int pmc_no;
  for (pmc_no = 0; pmc_no < kPmcsCount; pmc_no++) {
    // take config value if given else just write 0 to hopefully prevent overflows
    uint64_t threshold = kPmcThresholds[pmc_no] ? kPmcThresholds[pmc_no]
                                                : (1ULL << kFixedPmcMaxWidth);
    uint64_t pmc_value = (1ULL << kFixedPmcMaxWidth) - threshold;
    msr_write(PMC0 + pmc_no, pmc_value);
    printk(KERN_INFO "iguard: Threshold for PMC%d -> 0x%llx (PMC Value: 0x%llx)\n", 
           pmc_no, 
           threshold, 
           pmc_value);
  }

  // uint64_t msr_val = msr_read(FIXED_CTR0);
  // printk(KERN_INFO "Programmed CTR0: %llu\n", msr_val);

  // msr_val = msr_read(FIXED_CTR0 + 1);
  // printk(KERN_INFO "Programmed CTR1: %llu\n", msr_val);

  //uint64_t msr_val = msr_read(PMC0);
  //printk(KERN_INFO "Programmed PMC0: %llu\n", msr_val);

  // msr_val = msr_read(PMC0 + 1);
  // printk(KERN_INFO "Programmed PMC1: %llu\n", msr_val);

  printk(KERN_INFO "iguard: Starting guarding on behalf of UID %d with PID %d (%s)\n",
         kActiveUid, kActiveTask->pid, kActiveTask->comm);
  kNmiHandlerCalled = 0;
  kGuardingActive = 1;
  enable_perf_globally();
}

static void stop_guarding(void) {
  // Perf counters are running here! (Only visible if counting in kernel mode is
  // enabled)

  disable_perf_globally();

  // kGuardingActive is set to 0 if the handler was called during the guarding phase
  // thus we need to check for that too
  if ((kGuardingActive && kActiveTask->pid == current->pid) || kNmiHandlerCalled) {
    uint64_t pmc_val = msr_read(FIXED_CTR0);
    //printk(KERN_INFO "CTR0: %llu\n", pmc_val);
    pmc_val = msr_read(FIXED_CTR0 + 1);
    //printk(KERN_INFO "CTR1: %llu\n", pmc_val);
    pmc_val = msr_read(PMC0);
    //printk(KERN_INFO "PMC0: %llu\n", pmc_val);
    pmc_val = msr_read(PMC0 + 1);
    //printk(KERN_INFO "PMC1: %llu\n", pmc_val);

    kGuardingActive = 0;
    printk(KERN_INFO "iguard: Stopped guarding on behalf of UID %d with PID %d (%s)\n", 
      current_uid().val, current->pid, current->comm);
  } else {
    if (!kGuardingActive) {
      printk(KERN_ERR "iguard: Stop Guarding called before Start Guarding\n");
    } else {
      printk(KERN_ERR "iguard: Stop was required from wrong process! Got %d (%s); expected %d (%s)\n", 
        current->pid, current->comm, kActiveTask->pid, kActiveTask->comm);
    }
    enable_perf_globally();
  }

  if (kNmiHandlerCalled) {
    printk(KERN_INFO "iguard: NMI-handler was called during guarding\n");
    kNmiHandlerCalled = 0;
  }
}

static void update_config(struct iguard_request_args request_args) {
  int pmc_number = request_args.pmc_number;
  int event_selector = request_args.event_selector;
  int umask = request_args.umask;
  int threshold = request_args.threshold;
  

  if (pmc_number >= kPmcsCount || pmc_number >= MAX_PMCS) {
    printk(KERN_ERR "iguard Given PMC number (%d) exceeded maximum supported PMCs (%d)\n", pmc_number, kPmcsCount);
  }

  printk(KERN_INFO "iguard: Updating config (%d-%d-%d-%d)\n",
         pmc_number,
         event_selector,
         umask,
         threshold);

  // program the PMC accordingly
  configure_pmc(pmc_number, event_selector, umask);

  // store the given threshold for use during the guarding phase
  kPmcThresholds[pmc_number] = threshold;
}

static long device_ioctl(struct file *file, unsigned int ioctl_num,
                         unsigned long ioctl_param) {

  printk(KERN_WARNING "Request: %d\n", ioctl_num);
  switch (ioctl_num) {
    case CMD_START_PROFILING:
      start_profiling();
      break;
    case CMD_STOP_PROFILING: {
      struct iguard_pmc_results* userspace_pmc_results = (struct iguard_pmc_results*) ioctl_param;
      stop_profiling(userspace_pmc_results);
      break;
    }
    case CMD_START_GUARDING:
      printk(KERN_WARNING "Starting Guarding\n");
      start_guarding();
      break;
    case CMD_STOP_GUARDING:
      printk(KERN_WARNING "Stopping Guarding\n");
      stop_guarding();
      break;
    case CMD_UPDATE_CONFIG: {
      struct iguard_request_args request_args;
      from_user(&request_args, (void*) ioctl_param, sizeof(request_args));
      update_config(request_args);
      break;
      }
    default:
      printk(KERN_ERR "iguard: Unknown API command: %d\n", ioctl_num);
      return -1;
  }

  printk("Returning from Request %d\n", ioctl_num);
  return 0;
}

static int device_open(struct inode* inode, struct file* file) {
  // Lock module
  try_module_get(THIS_MODULE);
  return 0;
}

static int device_release(struct inode* inode, struct file* file) {
  // Unlock module
  module_put(THIS_MODULE);
  return 0;
}

static struct file_operations f_ops = {.unlocked_ioctl = device_ioctl,
                                       .open = device_open,
                                       .release = device_release};

static struct miscdevice misc_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = API_DEV_NAME,
    .fops = &f_ops,
    .mode = S_IRWXUGO,
};


static int guarded_core_is_isolated(void) {
  if (cpu_online(GUARDED_CPU_CORE)) {
    printk(KERN_INFO "CPU1 is online\n");
  }
  if (cpu_active(GUARDED_CPU_CORE)) {
    printk(KERN_INFO "CPU1 is active\n");
  }
  if (cpu_present(GUARDED_CPU_CORE)) {
    printk(KERN_INFO "CPU1 is present\n");
  }
  if (cpu_possible(GUARDED_CPU_CORE)) {
    printk(KERN_INFO "CPU1 is possible\n");
  }

  return cpu_online(GUARDED_CPU_CORE) == 0;
}

static int __init kmodule_init(void) {
  int r = misc_register(&misc_dev);
  if (r != 0) {
    printk(KERN_ERR "Failed registering device with %d\n", r);
    return 1;
  }

  if (check_cpuid()) {
    printk(KERN_ERR "iguard: CPUID failed -> Likely non-Intel CPU detected!\n");
  }

  //if (guarded_core_is_isolated()) {
  //  printk(KERN_ERR "iguard: iguard does not work when the guarded core (currently: %d) is isolated. Consider changing the core isolation or reconfigure the guarded core in iguardefender-common.h", GUARDED_CPU_CORE);
  //  return 1;
  //}

  //execute_on_guarded_cpu(init_pmc_configurations, NULL);
  init_pmc_configurations();

  // NMI_FLAG_FIRST prioritzes our handler over *every other NMI handler*. 
  // we're currently doing that to ensure that GLOBAL_STATUS is not reset by other handlers before
  // our execution.
  register_nmi_handler(NMI_LOCAL, pmi_event_handler, NMI_FLAG_FIRST, "PMI-iguard");
  printk(KERN_INFO "iguard: Registered NMI handler\n");
  return 0;
}

static void __exit kmodule_exit(void) {
  printk(KERN_INFO "iguard: Unloading kernel module... \n");

  unregister_nmi_handler(NMI_LOCAL, "PMI-iguard");
  printk(KERN_INFO "iguard: Unregistered NMI handler\n");

  //remove_proc_entry(PROC_API_FNAME, NULL);
  misc_deregister(&misc_dev);
  printk(KERN_INFO "iguard: Exit routine finished.\n");
}

module_init(kmodule_init);
module_exit(kmodule_exit);
