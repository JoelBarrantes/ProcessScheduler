#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H
 

extern int GLOBAL_PID; 
 
struct PCB;
struct process;
struct queue;
struct thread_args;




void *CPU_scheduler_thread(void *status);
int runCPUScheduler();
 
void *Job_scheduler_thread(void *status);
int runJobScheduler();


int select_process(int algorithm, int pid1, int pid2, int burst1, int burst2, int priority1, int priority2);
 
void *read_key(void *status);

int display_queue(struct queue *ready_queue);

int main();
 
 
#endif
