CC			= gcc
CFLAGS 		= -Wall -Wextra -O0 -g
TARGETS 	= sender attacker

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(TARGETS)
	rm *.dSYM

.PHONY: all clean
