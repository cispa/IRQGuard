// needed for utils.h stuff
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "libsc.h"

// comment in to test against the iguard
// #define iguard_ENABLE_PROTECTION
//#define ENABLE_ATTACK

#define BITS_TO_ATTACK 1000

// Attack vars
int thresh;
libsc_t* ctx;
libsc_eviction_set_t set;


#ifdef iguard_ENABLE_PROTECTION
#include "./iguardV3/iguard-api.h"
#endif

// must be included after iguard stuff
#include "utils.h"

// synchronization variables
_Atomic(int) attacker_go = 0;
_Atomic(int) attacker_rdy = 0;
_Atomic(int) victim_finished = 0;

__attribute__((aligned(4096*16)))
void one_bit() {
    // attention: flushing the same stream twice leads to deadlocks
    //   as we hold IO locks when iguard interrupts the process
    fflush(stdin);
    asm volatile("nop");
}

__attribute__((aligned(4096)))
void zero_bit() {
    // attention: flushing the same stream twice leads to deadlocks
    //   as we hold IO locks when iguard interrupts the process
    fflush(stdout);
    asm volatile("nop");
}

void delay() {
  //usleep(50000);
  while (!attacker_rdy) {}
  attacker_rdy = 0;
}

// ==============
// Victim Process
// ==============
__attribute__((aligned(4096)))
void victim(int N) {
  uint8_t secret = 0b01101001;
  size_t idx = 0; 
  int i = 0;
#ifdef iguard_ENABLE_PROTECTION
  if (iguard_no_attack()) {
  iguard_start_protection();
#endif
  while (i < N) {
    // signal the attacker to run
    attacker_go = 1;

    if ((secret >> idx) & 1) {
      //printf("accessing 1\n");
      for (int i = 0; i < 1000; i++) {
        one_bit();
      }
    } else {
      //printf("accessing 0\n");
      for (int i = 0; i < 1000; i++) {
        zero_bit();
      }
    }

    idx = (idx + 1) % 8;
    i += 1;
    //attacker_go = 1;
    delay();
  }
#ifdef iguard_ENABLE_PROTECTION
  iguard_stop_protection();
  } else {
    iguard_stop_protection();
    char buf[200];
    sprintf(buf, "[!!]Attack discovered after %d bits\n", i);
    print_characters_nolock(buf, strlen(buf));
  }
#endif

  victim_finished = 1;
}

void init() {
  //get_colocated_core_placement();
  // config works for lab17
  VICTIM_CORE = 1;
  ATTACKER_CORE = 2;
  // initialize your stuff
}


// ================
// Attacker Process
// ================
char leakage[BITS_TO_ATTACK+2] = {0};
size_t leakage_idx = 0;

int leakage_raw[BITS_TO_ATTACK+2] = {0};
size_t leakage_raw_idx = 0;

void* attacker(int N) {
  set_cpu_affinity(ATTACKER_CORE);

  // we try to leak a string that is twice as long. 
  // that should be enough to account for an attacker missing the first few bits
  while (leakage_raw_idx < N) {
  //while (!victim_finished) {
  attacker_rdy = 1;
  // synchronize attacker and victim
  while (attacker_go == 0 && !victim_finished) {}
  attacker_go = 0;

  int hit_cnt = 0;
  for (int i = 0; i < 3000; i++) {
#ifdef ENABLE_ATTACK
    if (libsc_prime_probe(ctx,set) > thresh) {
      hit_cnt++;
    }
#endif
    sched_yield();
  }
  leakage_raw[leakage_raw_idx++] = hit_cnt;
  if (hit_cnt > 10) {
    leakage[leakage_idx++] = '1';
  } else {
    leakage[leakage_idx++] = '0';
  }
  //printf("hits:%d\n", hit_cnt);

  } // while (!victim_finished) {

  return NULL;
}

void handler(int arg) {
  printf("%s\n", leakage);
  exit(0);
}

int calibrate_tresh(libsc_t* ctx, libsc_eviction_set_t set){
  uint64_t hit = 0;
  uint64_t miss = 0;
  #define CALIBRATE_RUNS 100000
  for (size_t i = 0; i < CALIBRATE_RUNS; i++)
  {
    zero_bit();
    miss += libsc_prime_probe(ctx,set);
  }

  for (size_t i = 0; i < CALIBRATE_RUNS; i++)
  {
    one_bit();
    hit += libsc_prime_probe(ctx,set);
  }

  //printf("Hit:  %ld\n", hit/CALIBRATE_RUNS);
  //printf("Miss: %ld\n", miss/CALIBRATE_RUNS);
  //fflush(stdout);

  return miss/CALIBRATE_RUNS;
}



int main() {

  init();

  // execute attacker
  pthread_t atk_tid = {0};

  // Calibrate attack
  ctx = libsc_init();
  libsc_build_eviction_set_vaddr(ctx,&set, (size_t) one_bit);  
  thresh = calibrate_tresh(ctx,set)+200;

  pthread_create(&atk_tid, NULL, attacker, BITS_TO_ATTACK);

  // execute victim
  set_cpu_affinity(VICTIM_CORE);
#ifdef iguard_ENABLE_PROTECTION
  iguard_init();
  iguard_parse_pmc_config("./pmc_config.iguard");
#endif
  victim(BITS_TO_ATTACK);
  pthread_join(atk_tid, NULL);
  printf("%s\n", leakage);

  FILE* fd = fopen("leakage.csv", "w");
  char line[255];
  for (size_t i = 0; i < leakage_raw_idx; i++) {
    snprintf(line, 254, "%d;%d\n", i, leakage_raw[i]);
    fwrite(line, strlen(line), 1, fd);
  }
  fclose(fd);
  return 0;
}
