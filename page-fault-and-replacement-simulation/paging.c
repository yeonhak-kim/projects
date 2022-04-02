#include "paging.h"
#include "pagesim.h"
#include "page_splitting.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

 /* The frame table pointer. You will set this up in system_init. */
fte_t *frame_table;

/*  --------------------------------- PROBLEM 2 --------------------------------------
    Checkout PDF section 4 for this problem

    In this problem, you will initialize frame_table. This is the pointer to the first
    frame table entry.

    The frame table will be located at physical address 0 in our simulated
    memory. You will first assign the frame_table global variable to point to
    this location in memory. You should zero out this first frame table entry
    (remember, size of a frame table entry is PAGESIZE), in case for
    any reason physical memory is not clean.

    You should then mark the first entry in the frame table as protected. We do
    this because we do not want our free frame allocator to give out the frame
    used by the frame table.

    HINTS:
        You will need to use the following global variables:
        - mem: Simulated physical memory already allocated for you.
        - PAGE_SIZE: The size of one page.
        You will need to initialize (set) the following global variable:
        - frame_table: a pointer to the first entry of the frame table

    -----------------------------------------------------------------------------------
*/
void system_init(void) {
    /*
     * 1. Set the frame table pointer to point to the first frame in physical
     * memory. Zero out the memory used by the frame table.
     *
     * Address "0" in memory will be used for the frame table. This table will
     * contain n frame table entries (fte_t), where n is the number of
     * frames in memory. The frame table will be useful later if we need to
     * evict pages during page faults.
     */

    /* 
        uint8_t *mem : physical memory (global variable)
        frame_table points to first entry of the physical memory
    */
    frame_table = (fte_t*) mem;

    /*
        zero out frame table (initializes frame_table to 0)
    */
    memset(frame_table, 0, PAGE_SIZE);


    /*
     * 2. Mark the first frame table entry as protected.
     *
     * The frame table contains entries for all of physical memory,
     * however, there are some frames we never want to evict.
     * We mark these special pages as "protected" to indicate this.
     */
    frame_table -> protected = 1;
}

/*  --------------------------------- PROBLEM 3 --------------------------------------
    Checkout PDF section 4 for this problem

    This function gets called every time a new process is created.
    You will need to allocate a new page table for the process in memory using the
    free_frame function so that the process can store its page mappings. Then, you
    will need to store the PFN of this page table in the process's PCB. Finally,
    you will need to initialize the diskmap (the mapping of the process' VPNs
    to swap entries).

    HINTS:
        - Look at the pcb_t struct defined in pagesim.h to know what to set inside.
        - You are not guaranteed that the memory returned by the free frame allocator
        is empty - an existing frame could have been evicted for our new page table.
        - As in the previous problem, think about whether we need to mark any entries
        in the frame_table as protected after we allocate memory for our page table.
    -----------------------------------------------------------------------------------
*/
void proc_init(pcb_t *proc) {
    /*
     * 1. Call the free frame allocator (free_frame) to return a free frame for
     * this process's page table. You should zero-out the memory.
     */
    pfn_t free_pfn = free_frame(); // free physical frame number
    fte_t* free_fte = &frame_table[free_pfn]; // frame table entry for this process
    // In case the frame table entry is not empty, zero-out the entry
    memset(free_fte, 0, sizeof(fte_t));
    // In case the page table is not empty, zero-out the page table entries(not by entries but in unit of bytes for complete removal of trash memory)
    memset(&mem[free_pfn * PAGE_SIZE], 0, PAGE_SIZE);
    /*
     * 2. Update the process's PCB with the frame number
     * of the newly allocated page table.
     *
     * Additionally, mark the frame's frame table entry as protected. You do not
     * want your page table to be accidentally evicted.
     */
    proc -> saved_ptbr = free_pfn;
    free_fte -> protected = 1;
    free_fte -> mapped = 1;
    /*
     * 3. Initialize the process' diskmap
     */
    diskmap_init(proc);
}

/*  --------------------------------- PROBLEM 4 --------------------------------------
    Checkout PDF section 5 for this problem

    Swaps the currently running process on the CPU to another process.

    Every process has its own page table, as you allocated in proc_init. You will
    need to tell the processor to use the new process's page table.

    HINTS:
        - Look at the global variables defined in pagesim.h. You may be interested in
        the definition of pcb_t as well.
    -----------------------------------------------------------------------------------
 */
void context_switch(pcb_t *proc) {
    PTBR = proc -> saved_ptbr;
}

/*  --------------------------------- PROBLEM 5 --------------------------------------
    Checkout PDF section 6 for this problem

    Takes an input virtual address and returns the data from the corresponding
    physical memory address. The steps to do this are:

    1) Translate the virtual address into a physical address using the page table.
    2) Go into the memory and read/write the data at the translated address.

    Parameters:
        1) address     - The virtual address to be translated.
        2) rw          - 'r' if the access is a read, 'w' if a write
        3) data        - One byte of data to write to our memory, if the access is a write.
                         This byte will be NULL on read accesses.

    Return:
        The data at the address if the access is a read, or
        the data we just wrote if the access is a write.

    HINTS:
        - You will need to use the macros we defined in Problem 1 in this function.
        - You will need to access the global PTBR value. This will tell you where to
        find the page table. Be very careful when you think about what this register holds!
        - On a page fault, simply call the page_fault function defined in page_fault.c.
        You may assume that the pagefault handler allocated a page for your address
        in the page table after it returns.
        - Make sure to set the dirty bit in the page table entry if it's a write.
        - Make sure to update the stats variables correctly (see stats.h)
        - Make sure to update the timestamp field of the physical frame that's being accessed.
    -----------------------------------------------------------------------------------
 */
uint8_t mem_access(vaddr_t address, char rw, uint8_t data) {

    stats.accesses++;
    /* Split the address and find the page table entry.
       Remember to keep a pointer to the entry so you can modify it later. */
    vpn_t vpn = vaddr_vpn(address);
    pte_t* page_table = (pte_t*) &mem[PTBR * PAGE_SIZE] ; // PTBR is PFN(Physical Frame Number) 
    pte_t* page_table_entry = &page_table[vpn];
    /* If an entry is invalid, just page fault to allocate a page for the page table. */
    if (!(page_table_entry -> valid)) {
        page_fault(address);
    }
    /* Update the timestamp of the appropriate frame table entry with the provided get_current_timestamp function. */
    pfn_t pfn = page_table_entry -> pfn;
    fte_t* frame_table_entry = &frame_table[pfn];
    frame_table_entry -> referenced = 1;  
    /*
        The physical address will be constructed like this:
        -------------------------------------
        |     PFN    |      Offset          |
        -------------------------------------
        where PFN is the value stored in the page table entry.
        We need to calculate the number of bits are in the offset.

        Create the physical address using your offset and the page
        table entry.

        Also, find the frame table entry corresponding to the VPN,
        and make sure set any relevant values.
    */
    uint16_t offset = vaddr_offset(address);
    paddr_t physical_address = (paddr_t) (pfn * PAGE_SIZE) + offset;

    /* Either read or write the data to the physical address
       depending on 'rw' */

    if (rw == 'r') {
        stats.reads++;
    } else if (rw == 'w') {
        stats.writes++;
        mem[physical_address] = data;
        page_table_entry -> dirty = 1;
    }

    /* Return the data read/written */
    return mem[physical_address];
}

/*  --------------------------------- PROBLEM 8 --------------------------------------
    Checkout PDF section 8 for this problem

    When a process exits, you need to free any pages previously occupied by the
    process. Otherwise, every time you closed and re-opened Microsoft Word, it
    would gradually eat more and more of your computer's usable memory.

    To free a process, you must clear the "mapped" bit on every page the process
    has mapped. If the process has swapped any pages to disk, you must call
    swap_free() using the page table entry pointer as a parameter.

    Each terminated process must free its diskmap as well.

    You must also clear the "protected" bits for the page table itself.
    -----------------------------------------------------------------------------------
*/
void proc_cleanup(pcb_t *proc) {
    /* Look up the process's page table */
    pfn_t page_table_pfn = proc -> saved_ptbr;
    pte_t* page_table = (pte_t*) &mem[page_table_pfn * PAGE_SIZE];

    /* Iterate the page table and clean up each valid page */
    for (size_t i = 0; i < NUM_PAGES; i++) {
        pte_t* page_table_entry = &page_table[i];
        // if valid then free entry otherwise, page was never loaded or page has been evicted for other process's page usage
        if (page_table_entry -> valid) {
            pfn_t page_table_entry_pfn = page_table_entry -> pfn;

            page_table_entry -> valid = 0;
            page_table_entry -> dirty = 0;
            // is it necessary to zero-out other attributes as well?
            frame_table[page_table_entry_pfn].mapped = 0;   
        }
        if (swap_exists(page_table_entry)) {
            swap_free(page_table_entry);
        }
    }

    /* Free the diskmap */ 
    diskmap_free(proc);
    /* Free the page table itself in the frame table */
    frame_table[page_table_pfn].protected = 0;
    frame_table[page_table_pfn].mapped = 0;
}

#pragma GCC diagnostic pop
