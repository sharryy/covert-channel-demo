#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

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
        } else {
          flock(fd, LOCK_UN);
        }
      }

      printf("%c", c);

      i++;
    }

    next_frame += frame_size;
  }
}
