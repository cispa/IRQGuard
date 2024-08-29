#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <x86intrin.h>
#include <immintrin.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <unistd.h>
#include <dirent.h>



int get_hyperthread(int logical_core) {
  // shamelessly stolen from libsc
  char cpu_id_path[300];
  char buffer[16];
  snprintf(cpu_id_path, 300, "/sys/devices/system/cpu/cpu%d/topology/core_id", logical_core);

  FILE* f = fopen(cpu_id_path, "r");
  if(!f) return -1;
  volatile int dummy = fread(buffer, 16, 1, f);
  fclose(f);
  int phys = atoi(buffer);
  int hyper = -1;

  DIR* dir = opendir("/sys/devices/system/cpu/");
  if(!dir) return -1;
  struct dirent* entry;
  while((entry = readdir(dir)) != NULL) {
    if(entry->d_name[0] == 'c' && entry->d_name[1] == 'p' && entry->d_name[2] == 'u' && (entry->d_name[3] >= '0' && entry->d_name[3] <= '9')) {
      snprintf(cpu_id_path, 300, "/sys/devices/system/cpu/%s/topology/core_id", entry->d_name);
      FILE* f = fopen(cpu_id_path, "r");
      if(!f) return -1;
      dummy += fread(buffer, 16, 1, f);
      fclose(f);
      int logical = atoi(entry->d_name + 3);
      if(atoi(buffer) == phys && logical != logical_core) {
        hyper = logical;
        break;
      }
    }
  }
  closedir(dir);
  return hyper;
}

int VICTIM_CORE = -1;
int ATTACKER_CORE = -1;

void get_colocated_core_placement() {
  long number_of_cores = sysconf(_SC_NPROCESSORS_ONLN);
  // set victim core to highest core
  VICTIM_CORE = number_of_cores - 1;
  // set attacker core to victim's HT
  ATTACKER_CORE = get_hyperthread(VICTIM_CORE);
  printf("Attacker Core:\t%d\n", ATTACKER_CORE);
  printf("Victim Core:\t%d\n", VICTIM_CORE);
}

void set_cpu_affinity(int cpu) {
  cpu_set_t cpuset;

  CPU_ZERO(&cpuset);
  CPU_SET(cpu, &cpuset);
  int s = pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
  if (s != 0) {
    fprintf(stderr, "could not set cpu affinity (core %d)!\n", cpu);
    exit(-1);
  }
}
