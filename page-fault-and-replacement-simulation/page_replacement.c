#include "types.h"
#include "pagesim.h"
#include "paging.h"
#include "swapops.h"
#include "stats.h"
#include "util.h"

pfn_t select_victim_frame(void);


/*  --------------------------------- PROBLEM 7 --------------------------------------
    Checkout PDF section 7 for this problem

    Make a free frame for the system to use.

    You will first call the page replacement algorithm to identify an
    "available" frame in the system.

    In some cases, the replacement algorithm will return a frame that
    is in use by another page mapping. In these cases, you must "evict"
    the frame by using the frame table to find the original mapping and
    setting it to invalid. If the frame is dirty, write its data to swap
    and update the current process' diskmap.
 * ----------------------------------------------------------------------------------
 */
pfn_t free_frame(void) {
    pfn_t victim_pfn;

    /* Call your function to find a frame to use, either one that is
       unused or has been selected as a "victim" to take from another
       mapping. */
    victim_pfn = select_victim_frame();

    /*
     * If victim frame is currently mapped, we must evict it:
     *
     * 1) Look up the corresponding page table entry
     * 2) If the entry is dirty, write it to disk with swap_write()
     * 3) Mark the original page table entry as invalid
     * 4) Unmap the corresponding frame table entry
     *
     */
   
    if (frame_table[victim_pfn].mapped) {
        /*
         * victim page table can be found in victim_process.saved_ptbr
         * victim_process can be found in frame_table[victim_pfn]
         */

        fte_t* victim_frame_info = &frame_table[victim_pfn];
        vpn_t victim_page_number = victim_frame_info -> vpn;
    
        pcb_t* victim_pcb = victim_frame_info -> process;
        pfn_t victim_page_table_pfn = victim_pcb -> saved_ptbr;

        pte_t* victim_page_table = (pte_t*) &mem[victim_page_table_pfn * PAGE_SIZE];
        pte_t* victim_page_table_entry = &victim_page_table[victim_page_number];

        victim_page_table_entry -> valid = 0;
        if (victim_page_table_entry -> dirty) {
            uint8_t* victim_page = &mem[victim_pfn * PAGE_SIZE];
            swap_write(victim_page_table_entry, victim_page);
            diskmap_update(victim_page_number, victim_page_table_entry -> swap);
            stats.writebacks++;
            victim_page_table_entry -> dirty = 0;
            frame_table[victim_pfn].mapped = 0;
        }
    }

    /* Return the pfn */
    return victim_pfn;
}



/*  --------------------------------- PROBLEM 9 --------------------------------------
    Checkout PDF section 7, 9, and 11 for this problem

    Finds a free physical frame. If none are available, uses either a
    randomized or clocksweep algorithm to find a used frame for
    eviction.

    Return:
        The physical frame number of a free (or evictable) frame.

    HINTS: Use the global variables MEM_SIZE and PAGE_SIZE to calculate
    the number of entries in the frame table.
    ----------------------------------------------------------------------------------
*/
static pfn_t last_victim; // gobal pointer for tracking the last victim frame
pfn_t select_victim_frame() {
    /* See if there are any free frames first */
    size_t num_entries = MEM_SIZE / PAGE_SIZE;
    for (size_t i = 0; i < num_entries; i++) {
        if (!frame_table[i].protected && !frame_table[i].mapped) {
            return i;
        }
    }

    if (replacement == RANDOM) {
        /* Play Russian Roulette to decide which frame to evict */
        pfn_t last_unprotected = NUM_FRAMES;
        for (pfn_t i = 0; i < num_entries; i++) {
            if (!frame_table[i].protected) {
                last_unprotected = i;
                if (prng_rand() % 2) {
                    return i;
                }
            }
        }
        /* If no victim found yet take the last unprotected frame
           seen */
        if (last_unprotected < NUM_FRAMES) {
            return last_unprotected;
        }
    } else if (replacement == CLOCKSWEEP) {
        /* Implement a clocksweep page replacement algorithm here */
        for (pfn_t i = 0; i < 2 * num_entries; i++) {
            pfn_t frame_number = (last_victim + 1 + i) % num_entries;
            if (!frame_table[frame_number].protected) {
                if (frame_table[frame_number].referenced) {
                    frame_table[frame_number].referenced = 0;
                } else {
                    last_victim = frame_number;
                    return frame_number;
                }
            }

        }

    }

    /* If every frame is protected, give up. This should never happen
       on the traces we provide you. */
    panic("System ran out of memory\n");
    exit(1);
}
