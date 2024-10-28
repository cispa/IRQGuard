# IRQGuard

## Workflow Overview
iguard essentially needs the following steps:
- Add iguard-API calls to the program
- create empty config
---
- Create memtrace_active file
- Disable ASLR
- Profile with Intel pin
- Delete memtrace_active file
- Copy prefetch config to directory of target program
---
- Load iguard kernel module
- Create profiling-enable file
- Choose thresholds (add them to the config file)
- Delete profiling-enable file
---
- Program is now protected

## Dependencies
Our PoC only supports Intel x86 CPUs with atleast performance monitoring version 4.
To compile the kernel module, you need the headers for your specific kernel version, e.g., on Ubuntu installed via `apt install linux-image-$(uname -r) linux-headers-$(uname -r) gcc-12`.
Note that we currently do not support kernel version 6 and that the module is developed and tested using kernel 5.15.

## Usage
To make the previously mentioned steps easier to handle, things are wrapped in Python scripts.
Thus, to protect your application using iguard, you need the following steps:

### Step 1
1) Add iguard-API calls to your program:
  - `iguard_init`
  - `iguard_parse_iguard_config`
  - `iguard_start_protection`
  - `iguard_stop_protection`
  - `iguard_no_attack`
  - `iguard_cleanup` (optional)
  
  Initially, you need to call `iguard_init`. It is enough to call that function once at the process startup.
  Next, you want to load a configuration for the performance counters (see `pmc_config_default.iguard` for an example config).
  This configuration file is then loading by calling `iguard_parse_iguard_config`.
  The code to protect has to be wrapped around calls to`iguard_start/stop_protection`.
  Inside this block of code, i.e., between `iguard_start_protection` and `iguard_stop_protection`, a developer can call `iguard_no_attack` in an if-branch to check for an ongoing attack. For example:
  ```C
  iguard_init();
  iguard_parse_iguard_config("./pmc_config.iguard");
  iguard_start_protection();
  if (iguard_no_attack()) {
    // everything is going fine
    <confidential computation>
  } else {
    // an attack has been detected
    <handle an ongoing attack, e.g., abort or retry>
  }
  iguard_stop_protection();
  iguard_cleanup();
  ```

2) Compile your application.
---
### Step 2 (Optional step if you want to improve the thresholds by enabling memory prefetching)
1) Go to `iguard/memtracer/`
2) Download and compile Intel PIN by running `./iguard_build_pintool.sh`
3) Create Memorytrace of program by running `./iguard_create_memtrace.sh <your-program>`. This may require you to generate an empty PMC config by, for example, executing `touch pmc_config.iguard`. This name has to be replaced by the actual config name passed to `iguard_parse_config`.
4) The MemTracer will create a file called `prefetch_config.iguard` containing the memory accesses that need to be prefetched. Copy this file to the directory of your binary.
---
### Step 3
1) Enable profiling mode by running `./iguard_enable_profiling.sh`
2) Disable ASLR by running `./disable_aslr.sh`
3) Load the iguard kernel module by running `make && sudo insmod iguard.ko`
4) Create a PMC config, using an arbitrary threshold (will be updated in the next step). For an example of a PMC config read the file `./pmc_config_default.iguard` and look up the PMC configuration syntax below.
5)  Execute the program, observe the thresholds and update the threshold in your PMC config.
6)  Disable profiling mode by running `./iguard_disable_profiling.sh`
7)  You can now start your program normally (as long as the IRQGuard kernel module is loaded and your application will be protected by IRQGuard!

## PMC Configuration File
The PMC configuration file is a CSV file in the following format:
```
pmc-number;event-selector;umask;threshold
```
whereas the entries refer to:
- **pmc-number**: The PMC that you want to use, e.g., 0 for PMC0.
- **event-selector**: The Event Selector as defined by the Intel manual.
- **umask**: The UMASK as defined by the Intel manual.
- **threshold**: The number of events after which iguard should stop the program.
