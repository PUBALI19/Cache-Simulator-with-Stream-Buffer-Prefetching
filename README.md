One can download these files and do "make clean" and then "make" to run these files.
In this project, I have implemented a flexible cache and memory hierarchy simulator and used it to compare the performance, area, and energy of different memory hierarchy configurations, using a subset of the SPEC 2006 benchmark suite, SPEC 2017 benchmark suite, and/or microbenchmarks.
Configuarble parameters:
CACHE should be configurable in terms of supporting any cache size, associativity, and block size, specified at the beginning of simulation: 
SIZE: Total bytes of data storage. 
ASSOC: The associativity of the cache. (ASSOC = 1 is a direct-mapped cache.  ASSOC = # blocks in the cache = SIZE/BLOCKSIZE is a fully-associative cache.) 
BLOCKSIZE: The number of bytes in a block. 
There are a few constraints on the above parameters: 
1) BLOCKSIZE is a power of two and
2) the number of sets is a power of two. Note that ASSOC (and, therefore, SIZE) need not be a power of two. The number of sets is determined by the following equation:
#sets = SIZE/(ASSOC X BLOCKSIZE)
Replacement policy: CACHE uses the LRU (least-recently-used) replacement policy.
Write Policy: CACHE should use the WBWA (write-back + write-allocate) write policy. 
Write-allocate: A write that misses in CACHE will cause a block to be allocated in CACHE. Therefore, both write misses and read misses cause blocks to be allocated in CACHE. 
Write-back: A write updates the corresponding block in CACHE, making the block dirty. It does not update the next level in the memory hierarchy (next level of cache or memory). If a dirty block is evicted from CACHE, a writeback (i.e., a write of the entire block) will be sent to the next level in the memory hierarchy.
