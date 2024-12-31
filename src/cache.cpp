///////////////////////////////////////////////////////////////////////////////
// You will need to modify this file to implement part A and, for extra      //
// credit, parts E and F.                                                    //
///////////////////////////////////////////////////////////////////////////////

// cache.cpp
// Defines the functions used to implement the cache.

#include "cache.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// You may add any other #include directives you need here, but make sure they
// compile on the reference machine!

///////////////////////////////////////////////////////////////////////////////
//                    EXTERNALLY DEFINED GLOBAL VARIABLES                    //
///////////////////////////////////////////////////////////////////////////////

/**
 * The current clock cycle number.
 * 
 * This can be used as a timestamp for implementing the LRU replacement policy.
 */
extern uint64_t current_cycle;

/**
 * For static way partitioning, the quota of ways in each set that can be
 * assigned to core 0.
 * 
 * The remaining number of ways is the quota for core 1.
 * 
 * This is used to implement extra credit part E.
 */
extern unsigned int SWP_CORE0_WAYS;

unsigned long long temp = 0;

///////////////////////////////////////////////////////////////////////////////
//                           FUNCTION DEFINITIONS                            //
///////////////////////////////////////////////////////////////////////////////

// As described in cache.h, you are free to deviate from the suggested
// implementation as you see fit.

// The only restriction is that you must not remove cache_print_stats() or
// modify its output format, since its output will be used for grading.

/**
 * Allocate and initialize a cache.
 * 
 * This is intended to be implemented in part A.
 *
 * @param size The size of the cache in bytes.
 * @param associativity The associativity of the cache.
 * @param line_size The size of a cache line in bytes.
 * @param replacement_policy The replacement policy of the cache.
 * @return A pointer to the cache.
 */
Cache *cache_new(uint64_t size, uint64_t associativity, uint64_t line_size,
                 ReplacementPolicy replacement_policy)
{
    // TODO: Allocate memory to the data structures and initialize the required
    //       fields. (You might want to use calloc() for this.)

    // allocate cache
    Cache *cache = (Cache*) calloc(1, sizeof(Cache));

    // calculate number of sets and ways
    unsigned int sets = size / (line_size * associativity);
    unsigned int ways = associativity;
    
    // allocate cache set
    cache->set = (CacheSet*) calloc(sets, sizeof(CacheSet));
        
    for (unsigned int j = 0; j < sets; ++j)
    {
        // allocate cache line
        cache->set[j].line = (CacheLine*) calloc(associativity, sizeof(CacheLine));
    }

    // initialize values in the cache line
    cache->sets = sets;
    cache->ways = ways;

    // initialize replacement policy
    cache->replacement_policy = replacement_policy;

    return cache;
}

/**
 * Access the cache at the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to access.
 * @param line_addr The address of the cache line to access (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this access is a write.
 * @param core_id The CPU core ID that requested this access.
 * @return Whether the cache access was a hit or a miss.
 */
CacheResult cache_access(Cache *c, uint64_t line_addr, bool is_write,
                         unsigned int core_id)
{
    // TODO: Return HIT if the access hits in the cache, and MISS otherwise.
    // TODO: If is_write is true, mark the resident line as dirty.
    // TODO: Update the appropriate cache statistics.

    unsigned int set_index = line_addr % c->sets;
    unsigned long long tag = line_addr / c->sets;

    if (is_write)
        c->stat_write_access++;
    else
        c->stat_read_access++;

    for (unsigned int i = 0; i < c->ways; ++i)
    {
        // if line is "valid", "tag" is correct, "core id" is correct
        if (c->set[set_index].line[i].valid && c->set[set_index].line[i].tag == tag)
        {
            // if is_write is true, mark the resident line as dirty.
            if (is_write)
            {
                c->set[set_index].line[i].dirty = true;
            }
            
            // update LRU time
            c->set[set_index].line[i].last_access_time = current_cycle;

            // return HIT
            return HIT;
        }
    }

    // update miss
    if (is_write)
        c->stat_write_miss++;
    else
        c->stat_read_miss++;

    return MISS;
}

/**
 * Install the cache line with the given address.
 * 
 * Also update the cache statistics accordingly.
 * 
 * This is intended to be implemented in part A.
 * 
 * @param c The cache to install the line into.
 * @param line_addr The address of the cache line to install (in units of the
 *                  cache line size, i.e., excluding the line offset bits).
 * @param is_write Whether this install is triggered by a write.
 * @param core_id The CPU core ID that requested this access.
 */
void cache_install(Cache *c, uint64_t line_addr, bool is_write,
                   unsigned int core_id)
{
    // TODO: Use cache_find_victim() to determine the victim line to evict.
    // TODO: Copy it into a last_evicted_line field in the cache in order to
    //       track writebacks.
    // TODO: Initialize the victim entry with the line to install.
    // TODO: Update the appropriate cache statistics.

    unsigned int set_index = line_addr % c->sets;
    unsigned long long tag = line_addr / c->sets;
    unsigned int way = cache_find_victim(c, set_index, core_id);

    c->last_evicted_line = c->set[set_index].line[way];

    if (c->last_evicted_line.valid && c->last_evicted_line.dirty)
    {
        c->stat_dirty_evicts++;
    }

    // overwrite cache line with new data
    c->set[set_index].line[way].valid = true;
    c->set[set_index].line[way].dirty = is_write;
    c->set[set_index].line[way].tag = tag;
    c->set[set_index].line[way].last_access_time = current_cycle;
    c->set[set_index].line[way].core_id = core_id;   
}

void changePartition()
{
    unsigned long long update = 10000;

    if (current_cycle > (update + temp))
    {
        if ((L2_hit[0]/L2_access[0]) > (L2_hit[1]/L2_access[1]))
        {
            if (SWP_CORE0_WAYS < 15) 
                SWP_CORE0_WAYS++;
                
        }
        else if ((L2_hit[0]/L2_access[0]) < (L2_hit[1]/L2_access[1]))
        {
            if (SWP_CORE0_WAYS > 1)
                SWP_CORE0_WAYS--;
        }
        L2_access[0] = 1;
        L2_access[1] = 1;
        L2_hit[0] = 1;
        L2_hit[1] = 1;
        temp += update;

        //printf("%d\n", SWP_CORE0_WAYS);
    }
}

/**
 * Find which way in a given cache set to replace when a new cache line needs
 * to be installed. This should be chosen according to the cache's replacement
 * policy.
 * 
 * The returned victim can be valid (non-empty), in which case the calling
 * function is responsible for evicting the cache line from that victim way.
 * 
 * This is intended to be initially implemented in part A and, for extra
 * credit, extended in parts E and F.
 * 
 * @param c The cache to search.
 * @param set_index The index of the cache set to search.
 * @param core_id The CPU core ID that requested this access.
 * @return The index of the victim way.
 */
unsigned int cache_find_victim(Cache *c, unsigned int set_index,
                               unsigned int core_id)
{
    // TODO: Find a victim way in the given cache set according to the cache's
    //       replacement policy.

    // TODO: In part A, implement the LRU and random replacement policies.

    if (c->replacement_policy == LRU) // LRU
    {
        unsigned long long last_access = UINT64_MAX;
        
        unsigned int LRU = 0;

        for (unsigned int i = 0; i < c->ways; ++i)
        {
            if (c->set[set_index].line[i].valid == false)
            {
                return i;
            }
            else if ((c->set[set_index].line[i].valid == true) && (c->set[set_index].line[i].last_access_time < last_access))
            {
                last_access = c->set[set_index].line[i].last_access_time;
                LRU = i;
            }
        }
        return LRU;
    }
    else if (c->replacement_policy == RANDOM) // random
    {
        srand (time(NULL));
        return rand() % c->ways;
    }
    else if (c->replacement_policy == SWP)
    {
        unsigned long long last_access[2] = {UINT64_MAX, UINT64_MAX};
        uint64_t last_LRU[2] = {0, 0};
        unsigned int occupy[2] = {0, 0};

        for (unsigned int i = 0; i < c->ways; ++i)
        {
            // prefer invalid lines if any exists
            if (c->set[set_index].line[i].valid == false)
            {
                return i;
            }
            // track per-core LRU and per-ways used
            else if (c->set[set_index].line[i].valid == true)
            {
                occupy[c->set[set_index].line[i].core_id]++;
                if (c->set[set_index].line[i].last_access_time < last_access[c->set[set_index].line[i].core_id])
                {
                    last_access[c->set[set_index].line[i].core_id] = c->set[set_index].line[i].last_access_time;
                    last_LRU[c->set[set_index].line[i].core_id] = i;
                }
            }
        }

        // If the other core exceeds its LRU quota, evicts its LRU.

        // eviction for core 0
        if (core_id == 0)
        {
            if (occupy[0] >= SWP_CORE0_WAYS)
                return last_LRU[0];
            else
                return last_LRU[1];
        }
        else if (core_id == 1)
        {
            if (occupy[1] >= (c->ways - SWP_CORE0_WAYS))
                return last_LRU[1];
            else 
                return last_LRU[0];
        }
    }
    else if (c->replacement_policy == DWP)
    {
        unsigned long long last_access[2] = {UINT64_MAX, UINT64_MAX};
        uint64_t last_LRU[2] = {0, 0};
        unsigned int occupy[2] = {0, 0};


        for (unsigned int i = 0; i < c->ways; ++i)
        {
            // prefer invalid lines if any exists
            if (c->set[set_index].line[i].valid == false)
            {
                return i;
            }
            // track per-core LRU and per-ways used
            else if (c->set[set_index].line[i].valid == true)
            {
                occupy[c->set[set_index].line[i].core_id]++;
                if (c->set[set_index].line[i].last_access_time < last_access[c->set[set_index].line[i].core_id])
                {
                    last_access[c->set[set_index].line[i].core_id] = c->set[set_index].line[i].last_access_time;
                    last_LRU[c->set[set_index].line[i].core_id] = i;
                }
            }
        }

        changePartition();
        
        if (core_id == 0)
        {
            if (occupy[0] >= SWP_CORE0_WAYS)
                return last_LRU[0];
            else
                return last_LRU[1];
        }
        else if (core_id == 1)
        {
            if (occupy[1] >= (c->ways - SWP_CORE0_WAYS))
                return last_LRU[1];
            else 
                return last_LRU[0];
        }
    }

    return 0;
    
    // TODO: In part E, for extra credit, implement static way partitioning.
    // TODO: In part F, for extra credit, implement dynamic way partitioning.
}

/**
 * Print the statistics of the given cache.
 * 
 * This is implemented for you. You must not modify its output format.
 * 
 * @param c The cache to print the statistics of.
 * @param label A label for the cache, which is used as a prefix for each
 *              statistic.
 */
void cache_print_stats(Cache *c, const char *header)
{
    double read_miss_percent = 0.0;
    double write_miss_percent = 0.0;

    if (c->stat_read_access)
    {
        read_miss_percent = 100.0 * (double)(c->stat_read_miss) /
                            (double)(c->stat_read_access);
    }

    if (c->stat_write_access)
    {
        write_miss_percent = 100.0 * (double)(c->stat_write_miss) /
                             (double)(c->stat_write_access);
    }

    printf("\n");
    printf("%s_READ_ACCESS     \t\t : %10llu\n", header, c->stat_read_access);
    printf("%s_WRITE_ACCESS    \t\t : %10llu\n", header, c->stat_write_access);
    printf("%s_READ_MISS       \t\t : %10llu\n", header, c->stat_read_miss);
    printf("%s_WRITE_MISS      \t\t : %10llu\n", header, c->stat_write_miss);
    printf("%s_READ_MISS_PERC  \t\t : %10.3f\n", header, read_miss_percent);
    printf("%s_WRITE_MISS_PERC \t\t : %10.3f\n", header, write_miss_percent);
    printf("%s_DIRTY_EVICTS    \t\t : %10llu\n", header, c->stat_dirty_evicts);
}