## bgpperf was written as a bare bones way to measure BGP RIB-out I/O performance.

Supports IPv4 unicast.  

Usage:

hot_potato:~/bgpperf$ ./bgpperf 192.168.254.2 64512 -m 1448 -c 3  
Attempting to connect to 192.168.254.2 with a local ASN of 64512 and a MSS of 1448 for 3 cycles  
Cycle: 1 IBGP Established with peer 192.168.254.2 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 2 IBGP Established with peer 192.168.254.2 in ASN 64512 with hold_time of 180...Complete!  
Cycle: 3 IBGP Established with peer 192.168.254.2 in ASN 64512 with hold_time of 180...Complete!  
Completed running 3 cycles  
Average prefix count received per cycle 200000.00  
Average time (seconds) per cycle: 1.710008 Variance: 1.046279 Standard Deviation: 1.022878  
Average RIB-out per cycle 27661.013672 Variance: 635196992.000000 Standard Deviation: 25203.114728  
