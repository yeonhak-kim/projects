#include "paging.h"
#include "stats.h"

/* The stats. See the definition in stats.h. */
stats_t stats;

/*  --------------------------------- PROBLEM 10 --------------------------------------
    Checkout PDF section 10 for this problem

    Calculate any remaining statistics to print out.

    You will need to include code to increment many of these stats in
    the functions you have written for other parts of the project.

    Use this function to calculate any remaining stats, such as the
    average access time (AAT).

    You may find the #defines in the stats.h file useful.
    -----------------------------------------------------------------------------------
*/
void compute_stats() {
    double total_access_time = stats.accesses * MEMORY_READ_TIME;
    double total_write_time = stats.writebacks * DISK_PAGE_WRITE_TIME;
    double total_page_fault_delay = stats.page_faults * DISK_PAGE_READ_TIME;
    stats.aat = (total_access_time + total_write_time + total_page_fault_delay) / stats.accesses;
}
