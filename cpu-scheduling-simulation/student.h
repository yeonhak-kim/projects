/*
 * student.h
 * Multithreaded OS Simulation for CS 2200
 *
 * YOU WILL NOT NEED TO MODIFY THIS FILE
 *
 */

#pragma once

#include "os-sim.h"
#include "stdbool.h"

/* Type of scheduling algorithm */
typedef enum sched_algorithm_type {
    FCFS    = 0x00,
    PS     = 0x01,
    RR      = 0x02,
} sched_algorithm_t;

/* The pcb queue structure */
typedef struct {
    pcb_t *head;
    pcb_t *tail;
} queue_t;

/* Function declarations */
extern void idle(unsigned int cpu_id);
extern void preempt(unsigned int cpu_id);
extern void yield(unsigned int cpu_id);
extern void terminate(unsigned int cpu_id);
extern void wake_up(pcb_t *process);
