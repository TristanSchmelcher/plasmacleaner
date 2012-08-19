CC=gcc
CFLAGS=-O2 -Wall -Werror --std=gnu99

plasmacleaner: plasmacleaner.c
	$(CC) $(CFLAGS) -o $@ $^ $$(pkg-config --cflags --libs gtk+-3.0)

clean:
	rm -f plasmacleaner
