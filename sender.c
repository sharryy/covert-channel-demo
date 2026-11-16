#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#define MESSAGE "SUPER-SECRET == "
#define NS_PER_SEC 1000000000ULL
#define SLOT_TIME 500000000ULL

#define PRINT_NS(label, t)                                                     \
  printf("%s %llu.%09llu\n", label, (t) / NS_PER_SEC, (t) % NS_PER_SEC)

static uint64_t now_ns(void) {
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
    perror("clock_gettime");
  }

  return (uint64_t)(ts.tv_sec * NS_PER_SEC) + (uint64_t)ts.tv_nsec;
}

void sleep_until(uint64_t target) {
  uint64_t now = now_ns();

  if (now < target) {
    uint64_t diff = target - now;
    struct timespec ts = {.tv_sec = diff / NS_PER_SEC,
                          .tv_nsec = diff % NS_PER_SEC};
    nanosleep(&ts, NULL);
  }
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);

  unsigned long long frame_size = strlen(MESSAGE) * 8 * (SLOT_TIME);

  int fd = open("/tmp/cc.lock", O_CREAT | O_RDONLY, 0666);

  if (fd < 0) {
    perror("open");
    return -1;
  }

  uint64_t current = now_ns();
  uint64_t next_frame = (current / frame_size + 1) * frame_size;

  while (1) {
    size_t i = 0;

    while (i < strlen(MESSAGE)) {
      unsigned char c = MESSAGE[i];

      for (int j = 7; j >= 0; j--) {
        uint64_t slot = 8 * i + (7 - j);

        sleep_until(next_frame + slot * SLOT_TIME);

        if ((c >> j) & 1) {
          flock(fd, LOCK_EX);
          // printf("1");
        } else {
          flock(fd, LOCK_UN);
          // printf("0");
        }
      }

      // printf(" - sent char: %c (%d)", c, c);
      printf("%c", c);

      i++;
    }

    next_frame += frame_size;
  }
}
