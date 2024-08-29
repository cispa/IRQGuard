// needed for utils.h stuff
#define _GNU_SOURCE

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "cacheutils.h"

#define iguard_T 500

// comment in to test against the iguard
//#define iguard_ENABLE_PROTECTION
//#define ENABLE_ATTACK

#define BITS_TO_ATTACK 1000

// Macros
#define REP5(x) x x x x x
#define REP4(x) x x x x
#define REP8(x) x x x x x x x x
#define REP10(x) REP5(x) REP5(x)
#define REP64(x) REP8(x) REP8(x) REP8(x) REP8(x) REP8(x) REP8(x) REP8(x) REP8(x)
#define REP512(x) REP64(x) REP64(x) REP64(x) REP64(x) REP64(x) REP64(x) REP64(x) REP64(x)
#define REP4K(x) REP512(x) REP512(x) REP512(x) REP512(x) REP512(x) REP512(x) REP512(x) REP512(x)



#include "./iguardV3/iguard-api.h"

// must be included after iguard stuff
#include "utils.h"

// synchronization variables
_Atomic(int) attacker_go = 0;
_Atomic(int) attacker_rdy = 0;
_Atomic(int) victim_finished = 0;

__attribute__((aligned(4096)))
void one_bit() {
    //printf("1");
    //fflush(stdout);
    REP512(asm volatile("nop"););
    // prevent compiler optimizations
    asm volatile("nop");
}

__attribute__((aligned(4096)))
void zero_bit() {
    //printf("0");
    //fflush(stdout);
    REP512(asm volatile("nop"););
    // prevent compiler optimizations
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
  iguard_start_protection();
  //iguard_start_profiling();
  if (iguard_no_attack()) {
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
  } else {
    printf("[!!]Attack discovered after %d bits\n", i);
  }
  iguard_stop_protection();
  //iguard_stop_profiling();
#endif

  victim_finished = 1;
}

void init() {
  //get_colocated_core_placement();
  // config works for lab17
  VICTIM_CORE = 0;
  ATTACKER_CORE = 6;
  // initialize your stuff
  CACHE_MISS = detect_flush_reload_threshold();
}


// ================
// Attacker Process
// ================
char leakage[BITS_TO_ATTACK+2] = {0};
size_t leakage_idx = 0;

int leakage_raw[BITS_TO_ATTACK+2] = {0};
size_t leakage_raw_idx = 0;

void* attacker(int N) {
  iguard_inner__set_cpu_affinity(ATTACKER_CORE);

  // we try to leak a string that is twice as long. 
  // that should be enough to account for an attacker missing the first few bits
  while (leakage_raw_idx < N || !victim_finished) {
  //while (!victim_finished) {
  attacker_rdy = 1;
  // synchronize attacker and victim
  while (attacker_go == 0 && !victim_finished) {}
  attacker_go = 0;

  int hit_cnt = 0;
  for (int i = 0; i < 3000; i++) {
#ifdef ENABLE_ATTACK
    if (flush_reload(one_bit)) {
      hit_cnt++;
    }
#endif
    sched_yield();
  }
  leakage_raw[leakage_raw_idx++] = hit_cnt;
  //printf("hit_cnt %d", hit_cnt);
  if (hit_cnt > 30) {
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

int main() {

  init();

  // execute attacker
  pthread_t atk_tid = {0};


  pthread_create(&atk_tid, NULL, attacker, BITS_TO_ATTACK);

  // execute victim
  iguard_inner__set_cpu_affinity(VICTIM_CORE);
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
