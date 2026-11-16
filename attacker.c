#include "common.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <time.h>
#include <unistd.h>

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
          } else {
            perror("flock");
          }
        } else {
          character |= 0 << (7 - len);
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
