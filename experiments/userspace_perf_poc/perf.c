#define _GNU_SOURCE 1

#include <asm/unistd.h>
#include <fcntl.h>
#include <linux/perf_event.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "cacheutils.h"

long perf_event_open(struct perf_event_attr* event_attr, pid_t pid, int cpu, int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, event_attr, pid, cpu, group_fd, flags);
}

static jmp_buf iguard_ovf_buf;


static void iguard_event_handler(int signum, siginfo_t* info, void* ucontext) {
    if(info->si_code != POLL_HUP) {
        exit(EXIT_FAILURE);
    }
//     printf("sig!\n");
    ioctl(info->si_fd, PERF_EVENT_IOC_REFRESH, 1);
    longjmp(iguard_ovf_buf, 1);
}

typedef struct {
    int fd;
} iguard_t;

iguard_t* iguard_init(int type, int events) {
    iguard_t* ctx = (iguard_t*)malloc(sizeof(iguard_t));
    
    // signal handler
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_sigaction = iguard_event_handler;
    sa.sa_flags = SA_SIGINFO;
    if (sigaction(SIGIO, &sa, NULL) < 0) {
        printf("Failed to setup signal handler\n");
        return NULL;
    }

    // performance counter
    struct perf_event_attr pe;
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_HARDWARE;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = type;
    pe.disabled = 1;
    pe.sample_type = PERF_SAMPLE_IP;
    pe.sample_period = events;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    
    pid_t pid = 0;
    int cpu = -1;
    int group_fd = -1;
    unsigned long flags = 0;

    int fd = perf_event_open(&pe, pid, cpu, group_fd, flags);
    if (fd == -1) {
        printf("perf_event_open failed, did you run as root?\n");
        return NULL;
    }

    // event handler for overflows
    fcntl(fd, F_SETFL, O_NONBLOCK|O_ASYNC);
    fcntl(fd, F_SETSIG, SIGIO);
    fcntl(fd, F_SETOWN, getpid());
    
    ctx->fd = fd;
    return ctx;
}

void iguard_reset(iguard_t* ctx) {
    ioctl(ctx->fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(ctx->fd, PERF_EVENT_IOC_REFRESH, 1);
}

size_t iguard_stop(iguard_t* ctx) {
    ioctl(ctx->fd, PERF_EVENT_IOC_DISABLE, 0);
    long long counter;
    read(ctx->fd, &counter, sizeof(long long));
    return (size_t)counter;

}

void iguard_cleanup(iguard_t* ctx) {
    close(ctx->fd);
}

#define iguard_monitor(__iguard_ctx) !setjmp(iguard_ovf_buf)



char mem[1024 * 1024];
volatile int done = 0;
volatile int result = 0;

int main(int argc, char** argv)
{
    memset(mem, 1, sizeof(mem));
    
    iguard_t* ctx = iguard_init(PERF_COUNT_HW_CACHE_MISSES, 10);

    
    // cache everything that is accessed
    for(int i = 0; i < 128; i++) {
        *(volatile char*)(mem + i * (4096 + 64));
    }

#ifdef ATTACK
    // flush everything that is accessed
    for(int i = 0; i < 128; i++) {
        flush(mem + i * (4096 + 64));
    }
#endif
    
    // start monitoring
    
    if(iguard_monitor(ctx)) {
        iguard_reset(ctx);
        for(int i = 0; i < 64; i++) {
            *(volatile char*)(mem + i * (4096 + 64));
            result++;
        }
        done = 1;
    } else {
        printf("Too many events!\n");
    }

    // stop monitoring
    size_t events = iguard_stop(ctx);

    printf("Result: %d\n", result);
    
    printf("Events: %zd, success: %d\n", events, done);

    iguard_cleanup(ctx);
} 
