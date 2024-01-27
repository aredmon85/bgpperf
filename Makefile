CC=gcc
CFLAGS=-Wall -I.
bgpperf: bgpperf.c
	$(CC) $(CFLAGS) -o bgpperf bgpperf.c -lm
clean:
	rm -f bgpperf
