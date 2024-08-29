# IRQGuard

## Workflow
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

## Usage
To make the previously mentioned steps easier to handle, things are wrapped in Python scripts.
Thus, to protect your application using iguard, you need the following steps:
1) Add iguard-API calls to your program:
  - `iguard_init`
  - `iguard_parse_iguard_config`
  - `iguard_start_protection`
  - `iguard_stop_protection`
  - `iguard_no_attack`
  - `iguard_cleanup` (optional)
2) Compile your application.
---
3) Go to `iguard/memtracer/`
4) Download and compile Intel PIN by running `./iguard_build_pintool.sh`
5) Create Memorytrace of program by running `./iguard_create_memtrace.sh <your-program>`. This may require you to generate an empty PMC config by, for example, executing `touch pmc_config.iguard`. This name has to be replaced by the actual config name passed to `iguard_parse_config`.
6) The MemTracer will create a file called `prefetch_config.iguard` containing the memory accesses that need to be prefetched. Copy this file to the directory of your binary.
---
7) Enable profiling mode by running `./iguard_enable_profiling.sh`
8) Disable ASLR by running `./disable_aslr.sh`
9) Load the iguard kernel module by running `make && sudo insmod iguard.ko`
10) Create a PMC config, using an arbitrary threshold (will be updated in the next step). For an example of a PMC config scroll down in this README. A default config can be found as `./pmc_config_default.iguard`.
11) Execute the program, observe the thresholds and update the threshold in your PMC config.
12) Disable profiling mode by running `./iguard_disable_profiling.sh`
13) Have fun!

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
