## bgpperf was written as a bare bones way to measure BGP RIB-out I/O performance.

Supports IPv4 unicast.  

Usage:

hot_potato:~/$ ./bgpperf 192.168.254.1 64512 -m 1260 -c 10 -w 5  
Connecting to 192.168.254.1 with a local ASN of 64512 and a MSS of 1260 for 10 cycles with 5 seconds between each cycle  
Cycle: 1 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 2 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 3 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 4 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 5 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 6 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 7 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 8 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 9 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 10 IBGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180...Complete!  
Completed running 10 cycles  
Average prefix count received per cycle 81617.00  
Average time (seconds) per cycle: 0.935514 Variance: 0.000339 Standard Deviation: 0.018407  
Average RIB-out per cycle 14614.974609 Variance: 81182.796875 Standard Deviation: 284.925950  

