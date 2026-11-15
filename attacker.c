#include <_time.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <time.h>
#include <unistd.h>

#define NS_PER_SEC 1000000000ULL
#define SLOT_TIME 1000000000ULL

#define MESSAGE_LEN 1

#define PRINT_NS(label, t)                                                     \
  printf("%s %llu.%09llu", label, (t) / NS_PER_SEC, (t) % NS_PER_SEC)

static uint64_t now_ns(void) {
  struct timespec tp;

  if (clock_gettime(CLOCK_MONOTONIC, &tp) < 0) {
    perror("clock_gettime");
  }

  return (uint64_t)tp.tv_sec * NS_PER_SEC + (uint64_t)tp.tv_nsec;
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);

  int fd = open("/tmp/cc.lock", O_RDONLY | O_CREAT, 0666);

  if (fd < 0) {
    perror("open");
  }

  struct timespec slot = {.tv_sec = SLOT_TIME / NS_PER_SEC};

  unsigned long long frame_size = MESSAGE_LEN * 8 * SLOT_TIME;

  while (1) {
    uint64_t cur = now_ns();
    uint64_t frames = cur / frame_size;
    uint64_t next = (frames + 1) * frame_size;
    uint64_t wait = next - cur;

    printf("slot starting in: %llu.%09llu\n", wait / NS_PER_SEC,
           wait % NS_PER_SEC);

    struct timespec sleep = {.tv_sec = wait / NS_PER_SEC,
                             .tv_nsec = wait % NS_PER_SEC};

    nanosleep(&sleep, NULL);

    int len = 0;
    uint8_t ch = 0b00000000;
    uint64_t half = SLOT_TIME / 2;

    // sleep for T/2 to correct race, At the bounday following happens.
    // 1000ms sender -> sends
    // 1000ms receiver -> receives
    // This introducing errors if attacker senses before sender sends.
    struct timespec s = {.tv_sec = half / NS_PER_SEC,
                         .tv_nsec = half % NS_PER_SEC};

    nanosleep(&s, NULL);
    while (len < 8) {
      int lock = flock(fd, LOCK_NB | LOCK_EX);

      if (lock < 0) {
        if (errno == EWOULDBLOCK) {
          ch = ch | 1 << (7 - len);
          printf("1");
        } else {
          perror("flock");
        }
      } else {
        ch = ch | 0 << (7 - len);
        printf("0");
        flock(fd, LOCK_UN);
      }

      len++;
      nanosleep(&slot, NULL);
    }

    printf("\nreceived: %c\n", ch);
  }
}
