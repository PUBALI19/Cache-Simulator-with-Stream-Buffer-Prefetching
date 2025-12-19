#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <cmath>
#include <tuple>
#include <cstdint>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "sim.h"
using namespace std;


int main(int argc, char *argv[]) 
{
	FILE *fp;
	char *trace_file;
	cache_params_t params;
	char rw;
	uint32_t addr;
	int total_no_of_ref = 0;
	
	if (argc != 9) 
	{
	    printf("Error: Expected 8 command-line arguments but was provided %d.\n", (argc - 1));
	    exit(EXIT_FAILURE);
	}
	
	params.BLOCKSIZE = (uint32_t)atoi(argv[1]);
	params.L1_SIZE = (uint32_t)atoi(argv[2]);
	params.L1_ASSOC = (uint32_t)atoi(argv[3]);
	params.L2_SIZE = (uint32_t)atoi(argv[4]);
	params.L2_ASSOC = (uint32_t)atoi(argv[5]);
	params.PREF_N = (uint32_t)atoi(argv[6]);
	params.PREF_M = (uint32_t)atoi(argv[7]);
	trace_file = argv[8];
	
	fp = fopen(trace_file, "r");
	if (fp == (FILE *)NULL) {
	    printf("Error: Unable to open file %s\n", trace_file);
	    exit(EXIT_FAILURE);
	}
	
	printf("===== Simulator configuration =====\n");
	printf("BLOCKSIZE:  %u\n", params.BLOCKSIZE);
	printf("L1_SIZE:    %u\n", params.L1_SIZE);
	printf("L1_ASSOC:   %u\n", params.L1_ASSOC);
	printf("L2_SIZE:    %u\n", params.L2_SIZE);
	printf("L2_ASSOC:   %u\n", params.L2_ASSOC);
	printf("PREF_N:     %u\n", params.PREF_N);
	printf("PREF_M:     %u\n", params.PREF_M);
	printf("trace_file: %s\n", trace_file);
	printf("\n");
	
	int l1_no_of_sets = params.L1_SIZE / (params.BLOCKSIZE * params.L1_ASSOC);
	int l2_no_of_sets = (params.L2_SIZE == 0) ? 0 : params.L2_SIZE / (params.BLOCKSIZE * params.L2_ASSOC);
	
	cache_simulator cs_l1(l1_no_of_sets, params.L1_ASSOC);
	cache_simulator cs_l2(l2_no_of_sets, params.L2_ASSOC);
	
	// Configure prefetch: L1 prefetch when no L2, L2 prefetch when L2 exists
	if (params.L2_SIZE == 0) {
	    cs_l1.prefetch_config(params.PREF_N, params.PREF_M, params.BLOCKSIZE);
	    cs_l2.prefetch_config(0, 0, params.BLOCKSIZE);
	} else {
	    cs_l1.prefetch_config(0, 0, params.BLOCKSIZE);
	    cs_l2.prefetch_config(params.PREF_N, params.PREF_M, params.BLOCKSIZE);
	}
	
	std::vector<std::vector<int>> l2_arr_lru(l2_no_of_sets, std::vector<int>(params.L2_ASSOC));
	std::vector<std::vector<int>> l1_arr_lru(l1_no_of_sets, std::vector<int>(params.L1_ASSOC));
	
	for (int i = 0; i < l1_no_of_sets; ++i) 
	{
	    for (uint32_t j = 0; j < params.L1_ASSOC; ++j) 
	    {
	        l1_arr_lru[i][j] = j;
	    }
	}
	
	if (params.L2_SIZE != 0) 
	{
	    for (int k = 0; k < l2_no_of_sets; ++k) 
	    {
	        for (uint32_t l = 0; l < params.L2_ASSOC; ++l) 
	        {
	            l2_arr_lru[k][l] = l;
	        }
	    }
	}
	
	while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) 
	{
	    total_no_of_ref++;
	    cs_l1.l1_cache(params.BLOCKSIZE, params.L1_SIZE, params.L1_ASSOC, addr, rw, l1_arr_lru, l2_arr_lru, params.L2_SIZE, params.L2_ASSOC, cs_l2);
	}
	//for printing my set array in ascending order and printing my tag array in mru to lru order
	printf("===== L1 contents =====\n");
	
	vector<int> indices(cs_l1.arr_set.size());
	for (size_t i = 0; i < cs_l1.arr_set.size(); ++i)
	    indices[i] = i;
	
	sort(indices.begin(), indices.end(), [&](int a, int b) {
	    return cs_l1.arr_set[a] < cs_l1.arr_set[b];
	});
	
	vector<uint32_t> set_copy = cs_l1.arr_set;
	vector<vector<uint32_t>> tag_copy = cs_l1.tag_storage;
	vector<vector<char>> dirty_copy = cs_l1.arr_dirty;
	vector<vector<int>> lru_copy = l1_arr_lru;  // include your LRU array
	
	for (size_t j = 0; j < indices.size(); ++j) 
	{
	    cs_l1.arr_set[j] = set_copy[indices[j]];
	    cs_l1.tag_storage[j] = tag_copy[indices[j]];
	    cs_l1.arr_dirty[j] = dirty_copy[indices[j]];
	    l1_arr_lru[j] = lru_copy[indices[j]];
	}
	
	for (size_t i = 0; i < cs_l1.arr_set.size(); ++i) 
	{
	    vector<int> line_indices(params.L1_ASSOC);
	    for (uint32_t j = 0; j < params.L1_ASSOC; ++j)
	        line_indices[j] = j;
	
	    sort(line_indices.begin(), line_indices.end(), [&](int a, int b) {
	        return l1_arr_lru[i][a] < l1_arr_lru[i][b];  // higher = MRU
	    });
	
	    vector<uint32_t> sorted_tags;
	    vector<char> sorted_dirty;
	    vector<int> sorted_lru;
	
	    for (int p_index : line_indices) 
	    {
	        sorted_tags.push_back(cs_l1.tag_storage[i][p_index]);
	        sorted_dirty.push_back(cs_l1.arr_dirty[i][p_index]);
	        sorted_lru.push_back(l1_arr_lru[i][p_index]);
	    }
	
	    cs_l1.tag_storage[i] = sorted_tags;
	    cs_l1.arr_dirty[i] = sorted_dirty;
	    l1_arr_lru[i] = sorted_lru;
	}
	
	for (size_t i = 0; i < cs_l1.arr_set.size(); ++i) 
	{
	    printf("set %d\t:", cs_l1.arr_set[i]);
	    for (uint32_t j = 0; j < params.L1_ASSOC; ++j) {
	        printf("%x %c\t", cs_l1.tag_storage[i][j], cs_l1.arr_dirty[i][j]);
	    }
	    printf("\n");
	}
	
	 
	if (params.L2_SIZE != 0)
	{
	    printf("\n");
	    printf("===== L2 contents =====\n");
	       vector<int> l2_indices(cs_l2.arr_set.size());
	       for (size_t i = 0; i < cs_l2.arr_set.size(); ++i)
	           l2_indices[i] = i;
	       
	       sort(l2_indices.begin(), l2_indices.end(), [&](int a, int b) {
	           return cs_l2.arr_set[a] < cs_l2.arr_set[b];
	       });
	       
	       vector<uint32_t> l2_set_copy = cs_l2.arr_set;
	       vector<vector<uint32_t>> l2_tag_copy = cs_l2.tag_storage;
	       vector<vector<char>> l2_dirty_copy = cs_l2.arr_dirty;
	       vector<vector<int>> l2_lru_copy = l2_arr_lru; 
	       
	       for (size_t j = 0; j < l2_indices.size(); ++j) 
	       {
	           cs_l2.arr_set[j] = l2_set_copy[l2_indices[j]];
	           cs_l2.tag_storage[j] = l2_tag_copy[l2_indices[j]];
	           cs_l2.arr_dirty[j] = l2_dirty_copy[l2_indices[j]];
	           l2_arr_lru[j] = l2_lru_copy[l2_indices[j]];
	       }
	       
	       //sorting from mru to lru required for the tag printing
	       for (size_t i = 0; i < cs_l2.arr_set.size(); ++i) 
	       {
	           vector<int> l2_line_indices(params.L2_ASSOC);
	           for (uint32_t j = 0; j < params.L2_ASSOC; ++j)
	               l2_line_indices[j] = j;
	       
	           sort(l2_line_indices.begin(), l2_line_indices.end(), [&](int a, int b) {
	               return l2_arr_lru[i][a] < l2_arr_lru[i][b];  // higher = MRU
	           });
	       
	           vector<uint32_t> l2_sorted_tags;
	           vector<char> l2_sorted_dirty;
	           vector<int> l2_sorted_lru;
	       
	           for (int p_index : l2_line_indices) 
	           {
	               l2_sorted_tags.push_back(cs_l2.tag_storage[i][p_index]);
	               l2_sorted_dirty.push_back(cs_l2.arr_dirty[i][p_index]);
	               l2_sorted_lru.push_back(l2_arr_lru[i][p_index]);
	           }
	       
	           cs_l2.tag_storage[i] = l2_sorted_tags;
	           cs_l2.arr_dirty[i] = l2_sorted_dirty;
	           l2_arr_lru[i] = l2_sorted_lru;
	        }
	       
	        for (size_t i = 0; i < cs_l2.arr_set.size(); ++i) 
	    	{
	           printf("set %d\t:", cs_l2.arr_set[i]);
	           for (uint32_t j = 0; j < params.L2_ASSOC; ++j) 
	           {
	               printf("%x %c\t", cs_l2.tag_storage[i][j], cs_l2.arr_dirty[i][j]);
	           }
	           printf("\n");
	        }
	
	 }
	 printf("\n");
	 if (params.PREF_N > 0) 
	 {
		if (params.L2_SIZE == 0) 
	    	{
	    		cs_l1.print_stream_buff(); 
	    		printf("\n");
		} 
		else 
	    	{
	    		cs_l2.print_stream_buff(); 
	    		printf("\n");
		}
	 }
	
	
	
	double miss_rate = static_cast<double>(l1.write_miss + l1.read_miss) / (l1.read_hit + l1.write_hit + l1.read_miss + l1.write_miss + l1.sb_read_hits + l1.sb_write_hits);
	double l2_miss_rate = (l2.read_miss + l2.read_hit) == 0 ? 0.0 : static_cast<double>(l2.read_miss) / (l2.read_miss + l2.read_hit + l2.sb_read_hits);
	    
	printf("===== Measurements =====\n");
	printf("a. L1 reads: %d\n", l1.read_hit + l1.read_miss + l1.sb_read_hits);
	printf("b. L1 read misses: %d\n", l1.read_miss);
	printf("c. L1 writes: %d\n", l1.write_hit + l1.write_miss + l1.sb_write_hits);
	printf("d. L1 write misses: %d\n", l1.write_miss);
	printf("e. L1 miss rate: %.4f\n", miss_rate);
	printf("f. L1 write backs: %d\n", l1.writeback_to_lower_mem);
	printf("g. L1 prefetches: %d\n", l1.prefetch_req);
	printf("h. L2 reads(demand): %d\n", l2.read_hit + l2.read_miss + l2.sb_read_hits);
	printf("i. L2 read misses(demand): %d\n", l2.read_miss);
	printf("j. L2 reads (prefetch) : %d\n", l2.prefetch_reads_from_l1);
	printf("k. L2 read misses (prefetch): %d\n", l2.prefetch_misses_from_l1);
	printf("l. L2 writes: %d\n", l2.write_hit + l2.write_miss);
	printf("m. L2 write misses: %d\n", l2.write_miss);
	printf("n. L2 miss rate: %.4f\n", l2_miss_rate);
	printf("o. L2 write backs: %d\n", l2.writeback_to_lower_mem);
	printf("p. L2 prefetches: %d\n", l2.prefetch_req);
	
	if (params.L2_SIZE != 0)
	    printf("q. memory traffic: %d\n", l2.read_miss + l2.write_miss + l2.writeback_to_lower_mem + l2.prefetch_misses_from_l1 + l2.prefetch_req);
	else
	    printf("q. memory traffic: %d\n", l1.read_miss + l1.write_miss + l1.writeback_to_lower_mem + l1.prefetch_req);
	
	return 0;
}
