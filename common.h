#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define NS_PER_SEC 1000000000ULL
#define SLOT_TIME 100000ULL

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
