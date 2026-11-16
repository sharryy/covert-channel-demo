#include <_time.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <time.h>
#include <unistd.h>

#define NS_PER_SEC 1000000000ULL
#define SLOT_TIME 500000000ULL

#define MESSAGE_LEN 16

#define PRINT_NS(label, t)                                                     \
  printf("%s %llu.%09llu\n", label, (t) / NS_PER_SEC, (t) % NS_PER_SEC)

static uint64_t now_ns(void) {
  struct timespec ts;

  if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0) {
    perror("clock_gettime");
  }

  return (uint64_t)ts.tv_sec * NS_PER_SEC + (uint64_t)ts.tv_nsec;
}

void sleep_until(uint64_t target) {
  uint64_t now = now_ns();

  if (target > now) {
    uint64_t diff = target - now;

    struct timespec ts = {.tv_sec = diff / NS_PER_SEC,
                          .tv_nsec = diff % NS_PER_SEC};

    nanosleep(&ts, NULL);
  }
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);

  int fd = open("/tmp/cc.lock", O_RDONLY | O_CREAT, 0666);

  if (fd < 0) {
    perror("open");
  }

  unsigned long long frame_size = MESSAGE_LEN * 8 * SLOT_TIME;
  uint64_t current = now_ns();
  uint64_t next_frame = (current / frame_size + 1) * frame_size;

  while (1) {
    size_t received = 0;

    while (received < MESSAGE_LEN) {
      int len = 0;
      uint8_t character = 0b00000000;

      while (len < 8) {
        sleep_until(next_frame + (len * SLOT_TIME) + (SLOT_TIME / 2) +
                    (8 * received * SLOT_TIME));

        int lock = flock(fd, LOCK_NB | LOCK_EX);

        if (lock < 0) {
          if (errno == EWOULDBLOCK) {
            character |= 1 << (7 - len);
            // printf("1");
          } else {
            perror("flock");
          }
        } else {
          character |= 0 << (7 - len);
          // printf("0");
          flock(fd, LOCK_UN);
        }

        len++;
      }
      received++;
      printf("%c", character);
    }

    next_frame += frame_size;
  }
}
