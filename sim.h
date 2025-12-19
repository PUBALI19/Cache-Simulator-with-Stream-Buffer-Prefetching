#ifndef SIM_CACHE_H
#define SIM_CACHE_H

typedef 
struct {
   uint32_t BLOCKSIZE;
   uint32_t L1_SIZE;
   uint32_t L1_ASSOC;
   uint32_t L2_SIZE;
   uint32_t L2_ASSOC;
   uint32_t PREF_N;
   uint32_t PREF_M;
} cache_params_t;


//structure for all the cache parameters
struct cache_params 
{
    int read_hit = 0;
    int write_hit = 0;
    int read_miss = 0;
    int write_miss = 0;
    int writeback_to_lower_mem = 0;
    
    int prefetch_reads_from_l1 = 0;      
    int prefetch_misses_from_l1 = 0;     
    
    int prefetch_req = 0;  //prefetches issued       
    
    //hits on stream buffer
    int sb_read_hits = 0;
    int sb_write_hits = 0;
};

//creating 2 instances for l1 and l2
cache_params l1;
cache_params l2;

//actual cache_simulator class which consists of all sub functions for prefetching, l1_cache (l1 level main function), l2_cache (l2 level main function)and for parsing the 32 bits address 
class cache_simulator 
{
	public:
	int c_no_set;
	int c_assoc;
	std::vector<std::vector<int>> arr_valid;
	std::vector<std::vector<char>> arr_dirty;
	std::vector<std::vector<uint32_t>> tag_storage;
	std::vector<std::vector<uint32_t>> arr_dirty_addr;
	std::vector<uint32_t> arr_set;
	int index_set = 0;
	int index_tag = 0;
	
	// Prefetch configuration
	uint32_t pref_N = 0;
	uint32_t pref_M = 0;
	uint32_t block_size_local = 0;
	int blockoffset_bits_local = 0;
	
	struct streambuffer 
	{
	    bool valid = false;
	    std::vector<uint32_t> blocks;
	    int top = 0;
	    int rec = 0;
	};
	
	std::vector<streambuffer> stream_buff;
	
	cache_simulator(int no_set, int assoc)
    	: 	c_no_set(no_set),
      		c_assoc(assoc),
      		arr_valid(no_set, std::vector<int>(assoc, 0)),
      		arr_dirty(no_set, std::vector<char>(assoc, ' ')),
      		tag_storage(no_set, std::vector<uint32_t>(assoc, 0)),
      		arr_dirty_addr(no_set, std::vector<uint32_t>(assoc, 0)) {}

	//printing the stream buffer
	void print_stream_buff() 
	{
		if (pref_N == 0 || stream_buff.empty()) 
		{
	    		return;
		}
	
		printf("===== Stream Buffer(s) contents =====\n");
	
		std::vector<int> indices(pref_N);
		for (int i = 0; i < (int)pref_N; ++i) 
		{
		    indices[i] = i;
		}
		
		//for sorting the array from mru to lru
		std::sort(indices.begin(), indices.end(), [this](int a, int b) 
		{
		    return stream_buff[a].rec < stream_buff[b].rec;
		});
		
		for (int p_index : indices) 
		{
		    streambuffer &sb = stream_buff[p_index];
		    
		    if (sb.valid) {
		        // for printing all the buffers
		        for (uint32_t i = 0; i < pref_M; ++i) {
		            uint32_t real_index = (sb.top + i) % pref_M;
		            printf("%x ", sb.blocks[real_index]);
		        }
		        printf("\n");
		    }
		}
	}
	
	//getting the number of sets and number of bits required for set and blockoffset 
	std::tuple<int, int, int> set_tag_bits_cal(int block_size, int cache_size, int assoc) 
	{
	    int no_of_sets = cache_size / (block_size * assoc);
	    int set_bits = std::log2(no_of_sets);
	    int blockoffset_bits = std::log2(block_size);
	    return {set_bits, blockoffset_bits, no_of_sets};
	}
	
	//parsing out the set and tag fields out of the 32 bits address
	std::pair<uint32_t, uint32_t> parseaddress(uint32_t address, int no_set_bits, int no_blockoffset_bits) 
 	{
	    uint32_t index_mask = (1u << no_set_bits) - 1;
	    uint32_t set = (address >> no_blockoffset_bits) & index_mask;
	    uint32_t tag = address >> (no_blockoffset_bits + no_set_bits);
	    return {tag, set};
	}
	
	//for implemetation of lru policy wherein counters are maintained indicating the recency
	//0: most recently used
	void lru_policy(int l_assoc, int l_index_set, int l_index_tag, std::vector<std::vector<int>>& l_arr_lru) 
	{
	    for (int q = 0; q < l_assoc; ++q) 
	    {
	        if (l_arr_lru[l_index_set][q] < l_arr_lru[l_index_set][l_index_tag]) 
		{
	            l_arr_lru[l_index_set][q] = l_arr_lru[l_index_set][q] + 1;
	        }
	    }
	    l_arr_lru[l_index_set][l_index_tag] = 0;
	}

	//small small functions for implementing prefetch
	void prefetch_config(uint32_t N, uint32_t M, uint32_t block_size) 
	{
	    pref_N = N;
	    pref_M = M;
	    block_size_local = block_size;
	    blockoffset_bits_local = block_size == 0 ? 0 : (int)std::log2(block_size);
	    
	    stream_buff.clear();
	    if (pref_N > 0 && pref_M > 0) {
	        stream_buff.resize(pref_N);
	        for (uint32_t i = 0; i < pref_N; ++i) 
		{
	            stream_buff[i].valid = false;
	            stream_buff[i].blocks.assign(pref_M, 0);
	            stream_buff[i].top = 0;
	            stream_buff[i].rec = (int)(pref_N - i);
	        }
	    }
	}
	
	void sb_mark_mru(int p_index) 
	{
	    if (pref_N == 0) return;
	    for (size_t i = 0; i < stream_buff.size(); ++i) 
	    {
	        if ((int)i != p_index) 
		{
	            stream_buff[i].rec += 1;
	        }
	    }
	    stream_buff[p_index].rec = 0;
	}
	
	int sb_hit(uint32_t block_number) 
	{
	    if (pref_N == 0) return -1;
	    int selected = -1;
	    int best_rec = INT32_MAX;
	    for (size_t i = 0; i < stream_buff.size(); ++i) {
	        if (!stream_buff[i].valid) continue;
	        for (uint32_t j = 0; j < pref_M; ++j) {
	            uint32_t real_index = (stream_buff[i].top + j) % pref_M;
	            if (stream_buff[i].blocks[real_index] == block_number) {
	                if (stream_buff[i].rec < best_rec) {
	                    selected = (int)i;
	                    best_rec = stream_buff[i].rec;
	                }
	                break;
	            }
	        }
	    }
	    return selected;
	}

	//for locating where the block is in stream buffer	
	int sb_pos_finder(int buf_p_index, uint32_t block_number) 
	{
	    if (buf_p_index < 0 || (uint32_t)buf_p_index >= stream_buff.size()) return -1;
	    for (uint32_t j = 0; j < pref_M; ++j) {
	        uint32_t real_index = (stream_buff[buf_p_index].top + j) % pref_M;
	        if (stream_buff[buf_p_index].blocks[real_index] == block_number) 
		{
	            return (int)j;
	        }
	    }
	    return -1;
	}
	
	//if it is a cache miss; bring in the required blocks into stream buffer
	void sb_miss(int buf_p_index, uint32_t start_block) 
	{
	    if (pref_N == 0) return;
	    streambuffer &sb = stream_buff[buf_p_index];
	    sb.valid = true;
	    sb.top = 0;
	    uint32_t next = start_block + 1;
	    for (uint32_t i = 0; i < pref_M; ++i) 
	    {
	        sb.blocks[i] = next + i;
	    }
	    sb_mark_mru(buf_p_index);
	}

	//extracting the block out on stream hit	
	void sb_hit_continue(int buf_p_index, int pos) 
	{
	    if (pref_N == 0) return;
	    streambuffer &sb = stream_buff[buf_p_index];
	    
	    std::vector<uint32_t> remaining;
	    for (uint32_t i = pos + 1; i < pref_M; ++i) 
	    {
	        uint32_t real_index = (sb.top + i) % pref_M;
	        remaining.push_back(sb.blocks[real_index]);
	    }
	    
	    uint32_t last_elem_index = (sb.top + pref_M - 1) % pref_M;
	    uint32_t next_prefetch = sb.blocks[last_elem_index] + 1;
	    
	    std::vector<uint32_t> newblocks;
	    for (auto v : remaining) newblocks.push_back(v);
	    while (newblocks.size() < pref_M) 
	    {
	        newblocks.push_back(next_prefetch);
	        next_prefetch++;
	    }
	    
	    for (uint32_t i = 0; i < pref_M; ++i) sb.blocks[i] = newblocks[i];
	    sb.top = 0;
	    sb_mark_mru(buf_p_index);
	}

	//when a block needs to be replaced in stream buffer; the least recently used one has to be evicted	
	int sb_lru() 
	{
	    if (pref_N == 0) return -1;
	    for (size_t i = 0; i < stream_buff.size(); ++i) 
	    {
	        if (!stream_buff[i].valid) return (int)i;
	    }
	    int best_p_index = 0;
	    int best_rec = stream_buff[0].rec;
	    for (size_t i = 1; i < stream_buff.size(); ++i) 
	    {
	        if (stream_buff[i].rec > best_rec) 
		{
	            best_rec = stream_buff[i].rec;
	            best_p_index = (int)i;
	        }
	    }
	    return best_p_index;
	}

	//main l2_cache function which is used for performing all the L2 functions. 
	//It gets called inside L1 function whenever l1 has a read or a write miss or if a dirty block needs to be evicted	
	void l2_cache(int l2_block_size, int l2_size, int l2_assoc, uint32_t l2_addr, char l2_rw, std::vector<std::vector<int>>& l2_arr_lru, bool is_prefetch) 
	{
	    auto [l2_set_bits, l2_blockoffset_bits, l2_no_of_sets] = set_tag_bits_cal(l2_block_size, l2_size, l2_assoc);
	    auto [l2_tag, l2_set] = parseaddress(l2_addr, l2_set_bits, l2_blockoffset_bits);
	
	    if (l2_rw == 'r') 
	    {
	        if (is_prefetch) 
		{
	            l2.prefetch_reads_from_l1++; 
	        }
	    }
	
	    bool l2_search_set = false;
	    bool l2_search_tag = false;

    	    //first search the arr_set to figure out if the index is matching or not	    
	    for (size_t k = 0; k < arr_set.size(); ++k) 
	    {
	        if (arr_set[k] == l2_set) 
		{
	            l2_search_set = true;
	            index_set = k;
	            break;
	        }
	    }
	    //if index doesn't match then add it to the arr_set
	    if (!l2_search_set) 
	    {
	        arr_set.push_back(l2_set);
	        index_set = arr_set.size() - 1;
	    }
	    //search through the tag_storage array to figure out if tag matched or not; hit and miss is decided based on this
	    for (int i = 0; i < l2_assoc; ++i) 
	    {
	        if (arr_valid[index_set][i] == 1 && l2_tag == tag_storage[index_set][i]) 
		{
	            l2_search_tag = true;
	            index_tag = i;
	            break;
	        }
	    }
	
	    //memory_block is determined
	    uint32_t block_number = (l2_addr >> l2_blockoffset_bits);
	    int sb_hit_p_index = -1;
	    if (pref_N > 0) 
	    {
	        sb_hit_p_index = sb_hit(block_number);
	    }
	
	    if (l2_search_tag) 
	    {
	        //it's a cache hit
	        lru_policy(l2_assoc, index_set, index_tag, l2_arr_lru);
	        if (l2_rw == 'r') 
		{
	            if (!is_prefetch) 
		    {
	                l2.read_hit++; //read demand hits are calculated
	            }
	        } 
		else 
		{
	            l2.write_hit++; //write demand hits are calculated
	            arr_dirty[index_set][index_tag] = 'D';
	        }
	        
	        //it's a cache hit and a stream buffer hit
	        if (sb_hit_p_index != -1) 
		{
	            int pos = sb_pos_finder(sb_hit_p_index, block_number);
	            if (pos >= 0) {
	                sb_hit_continue(sb_hit_p_index, pos);
	                uint32_t new_count = pos + 1;
	                streambuffer &sb = stream_buff[sb_hit_p_index];
	                uint32_t tail_start = (sb.top + pref_M - new_count) % pref_M;
	                for (uint32_t i = 0; i < new_count; i++) 
			{
			    //prefecth is requested
	                    l2.prefetch_req++; 
	                }
	            }
	        }
	    } 
	    else 
	    {
	        //it's a miss in cache
	        if (sb_hit_p_index == -1) 
		{
	            //it's a cache miss and a stream buffer miss
	            if (is_prefetch) 
		    {
	                l2.prefetch_misses_from_l1++; 
	            } 
		    else 
		    {
	                if (l2_rw == 'r')
	                    l2.read_miss++;
	                else
	                    l2.write_miss++;
	            }
	        } 
		else 
		{
	            //it's a miss in cache but a hit in stream buffer (now the block has to be brought in from stream buffer to cache
	            if (!is_prefetch) 
		    {
	                if (l2_rw == 'r') 
			{
	                    l2.sb_read_hits++;
	                } else 
			{
	                    l2.sb_write_hits++;
	                }
	            }
	        }
	        
	        for (int j = 0; j < l2_assoc; ++j) 
		{
	            if (l2_arr_lru[index_set][j] == (l2_assoc - 1)) 
		    {
	                if (arr_dirty[index_set][j] == 'D')
	                    l2.writeback_to_lower_mem++;
	
	                tag_storage[index_set][j] = l2_tag;
	                arr_dirty[index_set][j] = (l2_rw == 'w') ? 'D' : ' ';
	                arr_valid[index_set][j] = 1;
	                lru_policy(l2_assoc, index_set, j, l2_arr_lru);
	                break;
	            }
	        }
	
	        //modifying the stream buffer
	        if (sb_hit_p_index == -1) 
		{
	            //new stream needs to be made
	            if (pref_N > 0 && pref_M > 0 && !is_prefetch) 
		    {
	                int sbp = sb_lru();
	                if (sbp != -1) 
			{
	                    sb_miss(sbp, block_number);
	                    for (uint32_t i = 0; i < pref_M; ++i) {
	                        l2.prefetch_req++;
	                    }
	                }
	            }
	        } 
		else 
		{
	            int pos = sb_pos_finder(sb_hit_p_index, block_number);
	            if (pos >= 0) 
		    {
	                sb_hit_continue(sb_hit_p_index, pos);
	                uint32_t new_count = pos + 1;
	                for (uint32_t i = 0; i < new_count; i++) 
			{
	                    l2.prefetch_req++;
	                }
	            }
	        }
	    }
	}
	
	//this is the main l1 function in which all l1 operations take place.
	//based on the user read or write address and whether l2 level is present or not:
	//i) l1 cache miss; l2 is present; l2 prefetch is present: search through l2 cache and stream buffer
	//ii) l1 cache miss; l2 is absent; look for the block in stream buffer
	void l1_cache(int cache_block_size, int cache_size, int c_assoc, uint32_t l1_addr, char rw, std::vector<std::vector<int>>& arr_lru, std::vector<std::vector<int>>& l2_arr_lru,int l2_size, int l2_assoc, cache_simulator &cs_l2) 
	{
	    auto [cache_set_bits, cache_blockoffset_bits, c_no_of_sets] = set_tag_bits_cal(cache_block_size, cache_size, c_assoc);
	    auto [c_tag, c_set] = parseaddress(l1_addr, cache_set_bits, cache_blockoffset_bits);
	
	    bool search_set = false;
	    bool search_tag = false;
	
	    //first look for set/index matching
	    for (size_t k = 0; k < arr_set.size(); ++k) 
	    {
	        if (arr_set[k] == c_set) 
		{
	            search_set = true;
	            index_set = k;
	            break;
	        }
	    }
	    if (!search_set) 
	    {
	        arr_set.push_back(c_set);
	        index_set = arr_set.size() - 1;
	    }
	
	    //look for tag matching
	    for (int i = 0; i < c_assoc; ++i) 
	    {
	        if (arr_valid[index_set][i] == 1 && c_tag == tag_storage[index_set][i]) 
		{
	            search_tag = true;
	            index_tag = i;
	            break;
	        }
	    }
	
	    uint32_t block_number = (l1_addr >> cache_blockoffset_bits);
	    int sb_hit_p_index = -1;
	    if (pref_N > 0) 
	    {
	        sb_hit_p_index = sb_hit(block_number);
	    }
	
	    if (search_tag) 
	    {
	        //it's a cache hit
	        lru_policy(c_assoc, index_set, index_tag, arr_lru);
	        if (rw == 'r') 
		{
	            l1.read_hit++;
	        } else 
		{
	            l1.write_hit++;
	            arr_dirty[index_set][index_tag] = 'D';
	            arr_dirty_addr[index_set][index_tag] = l1_addr;
	        }
	
	        //it's a cache hit and a stream buffer hit
	        if (sb_hit_p_index != -1) 
		{
	            int pos = sb_pos_finder(sb_hit_p_index, block_number);
	            if (pos >= 0) 
		    {
	                sb_hit_continue(sb_hit_p_index, pos);
	                uint32_t new_count = pos + 1;
	                streambuffer &sb = stream_buff[sb_hit_p_index];
	                uint32_t tail_start = (sb.top + pref_M - new_count) % pref_M;
	                for (uint32_t i = 0; i < new_count; i++) 
			{
	                    uint32_t real_index = (tail_start + i) % pref_M;
	                    uint32_t blk = sb.blocks[real_index];
	                    uint32_t prefetch_addr = blk << cache_blockoffset_bits;
	                    l1.prefetch_req++;
	                    if (l2_size != 0) 
			    {
	                        cs_l2.l2_cache(cache_block_size, l2_size, l2_assoc, prefetch_addr, 'r', l2_arr_lru, true);
	                    }
	                }
	            }
	        }
	    } 
	    else 
	    {
	        //it's a cache miss
	        if (sb_hit_p_index == -1) 
		{
	            //it's a cache miss and a stream buffer miss
	            if (rw == 'r')
	                l1.read_miss++;
	            else
	                l1.write_miss++;
	
	            for (int j = 0; j < c_assoc; ++j) 
		    {
	                if (arr_lru[index_set][j] == (c_assoc - 1)) 
			{
	                    if (arr_dirty[index_set][j] == 'D') 
			    {
	                        l1.writeback_to_lower_mem++;
	                        if (l2_size != 0) 
				{
	                            cs_l2.l2_cache(cache_block_size, l2_size, l2_assoc, arr_dirty_addr[index_set][j], 'w', l2_arr_lru, false);
	                        }
	                    }
	
	                    if (l2_size != 0) 
			    {
	                        cs_l2.l2_cache(cache_block_size, l2_size, l2_assoc, l1_addr, 'r', l2_arr_lru, false);
	                    }
	
	                    tag_storage[index_set][j] = c_tag;
	                    lru_policy(c_assoc, index_set, j, arr_lru);
	                    arr_valid[index_set][j] = 1;
	                    if (rw == 'w') 
			    {
	                        arr_dirty[index_set][j] = 'D';
	                        arr_dirty_addr[index_set][j] = l1_addr;
	                    } else 
			    {
	                        arr_dirty[index_set][j] = ' ';
	                        arr_dirty_addr[index_set][j] = 0;
	                    }
	                    break;
	                }
	            }
	
	            //need new prefetch stream
	            if (pref_N > 0 && pref_M > 0) 
		    {
	                int sbp = sb_lru();
	                if (sbp != -1) 
			{
	                    sb_miss(sbp, block_number);
	                    for (uint32_t i = 0; i < pref_M; ++i) 
			    {
	                        uint32_t blk = stream_buff[sbp].blocks[i];
	                        uint32_t prefetch_addr = blk << cache_blockoffset_bits;
	                        l1.prefetch_req++;
	                        if (l2_size != 0) 
				{
	                            cs_l2.l2_cache(cache_block_size, l2_size, l2_assoc, prefetch_addr, 'r', l2_arr_lru, true);
	                        }
	                    }
	                }
	            }
	        } 
		else 
		{
	            //it's a cache miss and a stream buffer hit
	            if (rw == 'r') 
		    {
	                l1.sb_read_hits++;
	            } else 
		    {
	                l1.sb_write_hits++;
	            }
	
	            for (int j = 0; j < c_assoc; ++j) 
		    {
	                if (arr_lru[index_set][j] == (c_assoc - 1)) 
			{
	                    if (arr_dirty[index_set][j] == 'D') 
			    {
	                        l1.writeback_to_lower_mem++;
	                        if (l2_size != 0) 
				{
	                            cs_l2.l2_cache(cache_block_size, l2_size, l2_assoc, arr_dirty_addr[index_set][j], 'w', l2_arr_lru, false);
	                        }
	                    }
	
	                    if (rw == 'w' && l2_size != 0) 
			    {
	                        cs_l2.l2_cache(cache_block_size, l2_size, l2_assoc, l1_addr, 'r', l2_arr_lru, false);
	                    }
	
	                    tag_storage[index_set][j] = c_tag;
	                    lru_policy(c_assoc, index_set, j, arr_lru);
	                    arr_valid[index_set][j] = 1;
	                    if (rw == 'w') 
			    {
	                        arr_dirty[index_set][j] = 'D';
	                        arr_dirty_addr[index_set][j] = l1_addr;
	                    } else 
			    {
	                        arr_dirty[index_set][j] = ' ';
	                        arr_dirty_addr[index_set][j] = 0;
	                    }
	                    break;
	                }
	            }
	
	            int pos = sb_pos_finder(sb_hit_p_index, block_number);
	            if (pos >= 0) 
		    {
	                sb_hit_continue(sb_hit_p_index, pos);
	                uint32_t new_count = pos + 1;
	                streambuffer &sb = stream_buff[sb_hit_p_index];
	                uint32_t tail_start = (sb.top + pref_M - new_count) % pref_M;
	                for (uint32_t i = 0; i < new_count; i++) {
	                    uint32_t real_index = (tail_start + i) % pref_M;
	                    uint32_t blk = sb.blocks[real_index];
	                    uint32_t prefetch_addr = blk << cache_blockoffset_bits;
	                    l1.prefetch_req++;
	                    if (l2_size != 0) {
	                        cs_l2.l2_cache(cache_block_size, l2_size, l2_assoc, prefetch_addr, 'r', l2_arr_lru, true);
	                    }
	                }
	            }
	        }
	    }
	}
};


#endif
