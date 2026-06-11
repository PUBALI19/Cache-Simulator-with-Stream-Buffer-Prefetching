# Cache and Memory Hierarchy Simulator

A flexible, parameterized memory hierarchy and cache design simulator written in [C++] that models Multi-Level Cache Topologies (L1 and L2) augmented with an optional, highly configurable Stream-Buffer Hardware Prefetcher. 

This engine simulates the step-by-step behavior of cache allocations, evictions, hits, misses, and memory traffic tracking using real-world address traces generated from a subset of SPEC 2006, SPEC 2017, and standard microbenchmarks.

## Key Features & Specifications

- **Generic Cache Architecture**: Built using a modular object-oriented approach allowing individual cache instances (`CACHE`) to be dynamically configured, stacked, and instantiated at any level of the memory hierarchy.
- **Configurable Cache Geometry**: Supports arbitrary cache sizing (`SIZE`), block sizing (`BLOCKSIZE`), and set-associativity configurations (`ASSOC`) calculated via a power-of-two indexing scheme.
- **Replacement Policy**: Strictly enforces True Least-Recently-Used (LRU) tracking within each set using per-line age status counters.
- **Write Policy**: Implements Write-Back Write-Allocate (WBWA). Handles cascading dirty block evictions (writebacks) cleanly through structural hierarchy boundaries down to the main memory.
- **Stream-Buffer Prefetching**: Integrates a highly accurate $N$-way Stream Buffer Prefetch Unit utilizing circular-buffer tracking. Models complex scenario transitions (Demand Hits/Misses intersecting Prefetch Hits/Misses) and utilizes LRU recency matching to prune redundant overlapping prefetch paths.

## Memory Topologies Supported

The simulator is architected to dynamically resolve and model multiple custom structural topologies based on initial arguments:
1. **L1 Only**: CPU $\rightarrow$ L1 Cache $\rightarrow$ Main Memory
2. **Two-Level Hierarchy**: CPU $\rightarrow$ L1 Cache $\rightarrow$ L2 Cache $\rightarrow$ Main Memory
3. **Prefetching Augmented**: Integrates an $N$-stream buffer prefetch array tracking $M$ blocks deep, tied directly onto the last-level cache interface to measure Average Access Time (AAT) and memory bus traffic variations.

---

## Getting Started

### Prerequisites
- A standard Linux/Unix environment or terminal.
- GCC/G++ Compiler with support for standard build tools (or Java Development Kit if Java based).

### Installation & Compilation
The repository includes a strict optimization-enabled `Makefile`. To build the standalone executable (named `sim`):

```bash
make
```
Note: For debugging purposes via tools like GDB, modify flags to target -g without -O3 overrides.

### Execution Syntax
The compiled engine accepts exactly 8 sequential command-line arguments:
```bash
./sim <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>
```
### Argument Definitions:
BLOCKSIZE: Block size in bytes (Positive power-of-two integer, identical across all cache layers).

- L1_SIZE: Total size of the L1 cache in bytes.

- L1_ASSOC: Set-associativity of L1 (e.g., 1 for direct-mapped, or L1_SIZE/BLOCKSIZE for fully-associative).

- L2_SIZE: Total size of the L2 cache in bytes (0 disables the L2 cache tier completely).

- L2_ASSOC: Set-associativity of L2.

- PREF_N: Number of active Stream Buffers in the prefetch unit (0 disables prefetching).

- PREF_M: Number of consecutive memory blocks tracked inside each individual Stream Buffer.

- trace_file: Path to the targeted memory access log file.
### Execution Example:
To simulate a system with a 32-byte block size, an 8KB 4-way L1 cache, a 256KB 8-way L2 cache, a last-level prefetch unit with 3 stream buffers holding 10 blocks each, running the gcc trace:
```bash
./sim 32 8192 4 262144 8 3 10 traces/gcc_trace.txt
```
### Memory Trace Format
The simulator reads text traces logging real-time processor memory references. Address fields are processed as 32-bit hexadecimal unsigned integers matching the structural standard:
r ffe04540   # Read operation (Load) at address 0xffe04540
w 0eff2340   # Write operation (Store) at address 0x0eff2340
### Performance Tracking & Metrics
Every simulation execution evaluates structural health by outputing a detailed performance breakdown, including:
- Total layer reads, writes, and dirty block writebacks.
- Exact miss counters isolated against prefetch-buffer interventions.
- Standard Miss Rates (MRL1 and MRL2).
- Total Memory Traffic: A strict calculation summing physical transfers traversing the main memory controller bus boundary.
