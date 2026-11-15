#include <_time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#define MESSAGE "S"
#define NS_PER_SEC 1000000000ULL
#define SLOT_TIME 1000000000ULL

#define PRINT_NS(label, t)                                                     \
  printf("%s %llu.%09llu\n", label, (t) / NS_PER_SEC, (t) % NS_PER_SEC)

static uint64_t now_ns(void) {
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
    perror("clock_gettime");
  }

  return (uint64_t)(ts.tv_sec * NS_PER_SEC) + (uint64_t)ts.tv_nsec;
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);

  unsigned long long frame_size = strlen(MESSAGE) * 8 * (SLOT_TIME);

  int fd = open("/tmp/cc.lock", O_CREAT | O_RDONLY, 0666);

  if (fd < 0) {
    perror("open");
    return -1;
  }

  struct timespec slot = {.tv_sec = SLOT_TIME / NS_PER_SEC};
  struct timespec tp;

  while (1) {
    if (clock_gettime(CLOCK_MONOTONIC, &tp) == -1) {
      perror("clock_gettime");
      return -1;
    }

    uint64_t cur = (uint64_t)tp.tv_sec * NS_PER_SEC + (uint64_t)tp.tv_nsec;
    uint64_t frames = cur / frame_size;
    uint64_t next = (frames + 1) * frame_size;
    uint64_t wait = next - cur;

    // PRINT_NS("loop at", cur);
    // PRINT_NS("next frame", next);
    // PRINT_NS("wait", wait);

    struct timespec sleep = {.tv_sec = wait / NS_PER_SEC,
                             .tv_nsec = wait % NS_PER_SEC};

    printf("slot starting in: %llu.%09llu\n", wait / NS_PER_SEC,
           wait % NS_PER_SEC);
    nanosleep(&sleep, NULL);

    uint64_t start = now_ns();
    // PRINT_NS("started at", start);
    // PRINT_NS("difference", (start - next));

    size_t i = 0;

    while (i < strlen(MESSAGE)) {
      unsigned char c = MESSAGE[i];

      for (int j = 7; j >= 0; j--) {
        unsigned char bit = (c >> j) & 1;

        if (bit == 1) {
          flock(fd, LOCK_EX);
          printf("1");
          nanosleep(&slot, NULL);
        } else {
          flock(fd, LOCK_UN);
          printf("0");
          nanosleep(&slot, NULL);
        }
      }

      printf(" - sent char: %c (%d)\n", c, c);

      i++;
    }

    uint64_t end = now_ns();
    // PRINT_NS("ended at", end);
    // printf("\n\n");
  }
}
