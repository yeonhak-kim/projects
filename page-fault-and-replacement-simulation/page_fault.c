#include "paging.h"
#include "pagesim.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/*  --------------------------------- PROBLEM 6 --------------------------------------
    Checkout PDF section 7 for this problem

    Page fault handler.

    When the CPU encounters an invalid address mapping in a page table,
    it invokes the OS via this handler.

    Your job is to put a mapping in place so that the translation can
    succeed. You can use free_frame() to make an available frame.
    Update the page table with the new frame, and don't forget
    to fill in the frame table.

    When you access a frame, you should update its referenced bit.

    Lastly, you must fill your newly-mapped page with data. If the page has
    never been mapped before, you must read an empty page from disk space and
    update the diskmap. Otherwise, the data will ahve been swapped to the disk
    when the page was evicted last. Call swap_read() to pull the back in from
    swap space.

    HINTS:
         - You will need to use the global variable current_process when
           setting the frame table entry.

    ----------------------------------------------------------------------------------
 */
void page_fault(vaddr_t address) {
    /* First, split the faulting address and locate the page table entry.
       Remember to keep a pointer to the entry so you can modify it later. */
   stats.page_faults++;
   vpn_t vpn = vaddr_vpn(address);
   pte_t* page_table = (pte_t*) &mem[PTBR * PAGE_SIZE];
   pte_t* page_table_entry = &page_table[vpn];

    /* It's a page fault, so the entry obviously won't be valid. Grab
       a frame to use by calling free_frame(). */
   pfn_t free_pfn = free_frame();

    /* Update the page table entry. Make sure you set any relevant values. */
   page_table_entry -> pfn = free_pfn;
   page_table_entry -> dirty = 0;
   page_table_entry -> valid = 1;

    /* Update the frame table. Make sure you set any relevant values. */
   fte_t* frame_table_entry = &frame_table[free_pfn];
   frame_table_entry ->process = current_process;
   frame_table_entry -> mapped = 1;
   frame_table_entry -> vpn = vpn;
   
    /* Update the timestamp of the appropriate frame table entry with the provided get_current_timestamp function. */
   frame_table_entry -> referenced = 1;

    /* Initialize the page's memory. On a page fault, it is not enough
     * just to allocate a new frame. We must load in the old data from
     * disk into the frame. If there was no old data on disk, then
     * we need to clear out the memory (why?).
     *
     * 1) Get a pointer to the new frame in memory.
     * 2) If the page has swap set, then we need to load in data from memory
     *    using swap_read().
     * 3) Else, read a zeroed-out page from disk space and update the diskmap
     *    for the corresponding page.
     */
    uint8_t* page = &mem[free_pfn * PAGE_SIZE];
    if (swap_exists(page_table_entry)) {
       swap_read(page_table_entry, page);
    } else {
       diskmap_read(page_table_entry, page);
       diskmap_update(vpn, page_table_entry -> swap);
    }
}

#pragma GCC diagnostic pop
