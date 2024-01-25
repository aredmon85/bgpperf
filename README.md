bgpperf was written as a bare bones way to measure BGP RIB-out I/O performance.

Supports IPv4 unicast.  

Usage:
$:./bgpperf 192.168.254.1 64513 -m 1448
Attempting to connect to 192.168.254.1 with a local ASN of 64513 and a MSS of 1448 for 1 cycles
BGP Established with peer 192.168.254.1 in ASN 64512 with hold_time of 180
Keepalive received
Keepalive received
###EOR Received!!!###
66305 total updates received
4797673 total update bytes received
Total time: 1.275752
RIB-out (updates/second): 51973.269531
Total prefixes received: 385988
