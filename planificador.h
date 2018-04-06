#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H
 
#include <unistd.h>
#include <termios.h>

extern int GLOBAL_PID; 
extern int SECS;
 
struct PCB;
struct process;
struct queue;
struct thread_args;

int display_times(struct queue *done_queue);

void *CPU_scheduler_thread(void *status);
int runCPUScheduler();
 
void *Job_scheduler_thread(void *status);
int runJobScheduler();


int select_process(int algorithm, int pid1, int pid2, int burst1, int burst2, int priority1, int priority2);
 
void *read_key(void *status);

int display_queue(struct queue *ready_queue);


char getch();
int main();


 
 
#endif
