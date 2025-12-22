For compiling these files: "make clean" and then "make".  
To run these scripts:  
The simulator accepts exactly 8 command-line arguments in the following order:   
sim   <BLOCKSIZE> <L1_SIZE>  <L1_ASSOC> <L2_SIZE>  <L2_ASSOC> <PREF_N>  <PREF_M> <trace_file>  
- BLOCKSIZE: Positive integer.  Block size in bytes.  (Same block size for all caches in the memory hierarchy.)   
- L1_SIZE:   Positive integer.  L1 cache size in bytes.   
- L1_ASSOC:  Positive integer.  L1 set-associativity (1 is direct-mapped, L1_SIZE/BLOCKSIZE is fully-associative).  
- L2_SIZE:   Positive integer.  L2 cache size in bytes.  L2_SIZE = 0 signifies that there is no L2 cache.   
- L2_ASSOC:  Positive integer.  L2 set-associativity (1 is direct-mapped, L2_SIZE/BLOCKSIZE is fully-associative).   
- PREF_N:    Positive integer.  Number of Stream Buffers in the L1 prefetch unit (if there is no L2) or the L2 prefetch unit (if there is an L2).  PREF_N = 0 disables the prefetch unit.  
- PREF_M:    Positive integer.  Number of memory blocks in each Stream Buffer in the L1 prefetch unit (if there is no L2) or the L2 prefetch unit (if there is an L2).   
- trace_file:  Character string.  Full name of trace file including any extensions.
Outputs:
- Memory hierarchy configuration and trace filename.   
- The final contents of all caches and their stream buffers (if applicable). For cache contents, blocks within a set must be printed out from most-recently-used to least-recently-used. Omit (do not print) invalid blocks within a set, if there are any. If a given set has no valid blocks, omit (do not print) that set. Stream buffers (if present) must be printed out from most-recently-used to least-recently-used, and only valid stream buffers is printed out.
- The following measurements: (note that “miss” means neither the cache nor its stream buffers hit)   
a. number of L1 reads   
b. number of L1 read misses, excluding L1 read misses that hit in the stream buffers if L1 prefetch unit is enabled    
c. number of L1 writes   
d. number of L1 write misses, excluding L1 write misses that hit in the stream buffers if L1 prefetch unit is enabled    
e. L1 miss rate = MRL1 = (L1 read misses + L1 write misses)/(L1 reads + L1 writes)   
f. number of writebacks from L1 to next level   
g. number of L1 prefetches (prefetch requests from L1 to next level, if prefetch unit is enabled)      
h. number of L2 reads that did not originate from L1 prefetches (should match b+d: L1 read misses + L1 write misses)     
i. number of L2 read misses that did not originate from L1 prefetches, excluding such L2 read misses that hit in the stream buffers if L2 prefetch unit is enabled    
j. number of L2 reads that originated from L1 prefetches (should match g: L1 prefetches)   
k. number of L2 read misses that originated from L1 prefetches, excluding such L2 read misses that hit in the stream buffers if L2 prefetch unit is enabled   
l. number of L2 writes (should match f: number of writebacks from L1)   
m. number of L2 write misses, excluding L2 write misses that hit in the stream buffers if L2 prefetch unit is enabled  
n. L2 miss rate (from standpoint of stalling the CPU) = MRL2 = (item i)/(item h)   
o. number of writebacks from L2 to memory   
p. number of L2 prefetches (prefetch requests from L2 to next level, if prefetch unit is enabled) 
q. total memory traffic = number of blocks transferred to/from memory  
(with L2, should match i+k+m+o+p: all L2 read misses + L2 write misses + writebacks from L2 + L2 prefetches)  
(without L2, should match b+d+f+g: L1 read misses + L1 write misses + writebacks from L1 + L1 prefetches)    

In this project, I have implemented a flexible cache and memory hierarchy simulator and used it to compare the performance, area, and energy of different memory hierarchy configurations, using a subset of the SPEC 2006 benchmark suite, SPEC 2017 benchmark suite, and/or microbenchmarks.  
Configuarble parameters:  
- CACHE should be configurable in terms of supporting any cache size, associativity, and block size, specified at the beginning of simulation:  
- SIZE: Total bytes of data storage.   
- ASSOC: The associativity of the cache. (ASSOC = 1 is a direct-mapped cache.  ASSOC = # blocks in the cache = SIZE/BLOCKSIZE is a fully-associative cache.)   
- BLOCKSIZE: The number of bytes in a block.  
There are a few constraints on the above parameters: BLOCKSIZE is a power of two and the number of sets is a power of two. Note that ASSOC (and, therefore, SIZE) need not be a power of two.
The number of sets is determined by the following equation:  
#sets = SIZE/(ASSOC X BLOCKSIZE)  
Replacement policy: CACHE uses the LRU (least-recently-used) replacement policy.  
Write Policy: CACHE uses the WBWA (write-back + write-allocate) write policy.   
Write-allocate: A write that misses in CACHE will cause a block to be allocated in CACHE. Therefore, both write misses and read misses cause blocks to be allocated in CACHE.   
Write-back: A write updates the corresponding block in CACHE, making the block dirty. It does not update the next level in the memory hierarchy (next level of cache or memory). If a dirty block is evicted from CACHE, a writeback (i.e., a write of the entire block) will be sent to the next level in the memory hierarchy.  

