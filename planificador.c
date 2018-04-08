#include "planificador.h"
#include "list.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <termios.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_add

int ADDRESS = 8888;
int GLOBAL_PID=0;
int SECS = 0;
int SECS_L = 0;
int CPU_WAIT = 0;
pthread_mutex_t mutex;


struct PCB{
    int PID;   
    int priority;
    int status;
};

struct process
{
    struct PCB process_pcb;
    int *status;
    int burst;     
    int curr_burst;
	int begin_time;
    int TAT;
    int WT;
};

struct queue{
	struct process p;
	struct list_head list;
};

struct thread_args{
	int *status;
	struct queue *ready_queue;
	struct queue *done_queue;
	int algorithm;
	int quantum;
};
	

char getch() {
        char buf = 0;
        struct termios old = {0};
        if (tcgetattr(0, &old) < 0)
                perror("tcsetattr()");
        old.c_lflag &= ~ICANON;
        old.c_lflag &= ~ECHO;
        old.c_cc[VMIN] = 1;
        old.c_cc[VTIME] = 0;
        if (tcsetattr(0, TCSANOW, &old) < 0)
                perror("tcsetattr ICANON");
        if (read(0, &buf, 1) < 0)
                perror ("read()");
        old.c_lflag |= ICANON;
        old.c_lflag |= ECHO;
        if (tcsetattr(0, TCSADRAIN, &old) < 0)
                perror ("tcsetattr ~ICANON");
        return (buf);
}


void *count(void *args){
	
	int *finish = (int *)args;
	while (*finish != 1){
		pthread_mutex_lock(&mutex);	
		SECS++;
		printf("%d",SECS);
		fflush(stdout);		
		pthread_mutex_unlock(&mutex);
		sleep(1);
		
	}
	return NULL;
}


void *CPU_scheduler_thread(void *arguments){

	struct thread_args *args = (struct thread_args *)arguments;
    int *status_ptr = (int *)args->status;
	int algorithm = (int)args->algorithm;
	int quantum = (int)args->quantum;
	struct queue *p_queue = (struct queue *)args->ready_queue;
	struct queue *d_queue = (struct queue *)args->done_queue;
	
	int finish = 0;	
	//pthread_t counter;

	printf("CPU Scheduler ready. Waiting for a process...\n");
	fflush(stdout);
	/*	
	if(pthread_create(&counter, NULL, count, &finish)){
		fprintf(stderr, "Error creating thread\n");
		return NULL;
	}
	*/

	while(*status_ptr != 1){
	
		runCPUScheduler(p_queue,d_queue, algorithm, quantum);
		
	}

	finish = 1;
	/*pthread_join(counter, NULL);*/
	display_times(d_queue);	
	
	
	
	return NULL;
    
};

void *Job_scheduler_thread(void *arguments){
	
	struct thread_args *args = (struct thread_args *)arguments;
    int *status_ptr = (int *)args->status;
	struct queue *p_queue = (struct queue *)args->ready_queue;

	///////////////////////SOCKET///////////////////////

	int socket_desc;
    struct sockaddr_in server, client;

	int client_sock , c ;


    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket Created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( ADDRESS );
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
		*status_ptr = 1;
        return NULL;
    }
    puts("Bind Done");

	listen(socket_desc,1);

	//Accept and incoming connection
    puts("Waiting for a connection...");
    c = sizeof(struct sockaddr_in);
   
	//accept connection from an incoming client
	client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
	
	
	if (client_sock < 0)
	{
		*status_ptr = 1;
	    perror("accept failed");
	    return NULL;
	}
	puts("Connection established");
     

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 500000;
	
	setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (struct timeval *)&tv, sizeof(struct timeval));
	///////////////////////SOCKET///////////////////////


	while(*status_ptr != 1){
		
		int res = runJobScheduler(p_queue, client_sock);
		if (res == 2){
			printf("\nConnection lost\n");
			*status_ptr = 1;
		}
	}

	close(client_sock);
	close(socket_desc);
	return NULL;
    
};

void *read_key(void *arguments){

	struct thread_args *args = (struct thread_args *)arguments;
	struct queue *p_queue = (struct queue *)args->ready_queue;
    int *status_ptr = (int *)args->status;
	
	while(*status_ptr != 1){	
		int ch = getch();
	
		if (ch < 0){
				clearerr(stdin);
			if (ferror(stdin)){
			}
		}

		if (ch == 27){
			
			++(*status_ptr);
			printf("\nFinishing...\n");
			fflush(stdout);
			break;
			
		}
		if (ch == 9){

			display_queue(p_queue);
		}
	}
	return NULL;

};

int display_queue(struct queue *ready_queue){
	struct list_head *pos;
	struct queue *queue_tmp;
	printf("\nReady Queue: \n");
	list_for_each(pos, &ready_queue -> list){
		 queue_tmp = list_entry(pos, struct queue, list);
		 printf("PID: %d | Total Burst: %d | Remaining Burst: %d | Priority: %d\n",queue_tmp -> p.process_pcb.PID, queue_tmp -> 			 p.burst,  queue_tmp -> p.curr_burst, queue_tmp-> p.process_pcb.priority);
		 
	}
	printf("\n");
	fflush(stdout);
	return 0;

}

int display_times(struct queue *done_queue){
	
	//pthread_mutex_lock(&mutex);
	int p_quantity = 0;
	int cpu_wait = 0;	
	float mu_tat = 0;
	float mu_wt = 0;

	int total_burst = 0;	
	struct list_head *pos;
	struct queue *queue_tmp;
		
	printf("\nFinished Processes Table: \n");
	list_for_each(pos, &done_queue -> list){

		p_quantity++;
		queue_tmp = list_entry(pos, struct queue, list);
		printf("PID: %d | Burst: %d | Arrival Time: %d |Turn-Around Time: %d | Waiting Time: %d\n", queue_tmp -> p.process_pcb.PID, queue_tmp -> p.burst ,queue_tmp -> p.begin_time,  queue_tmp -> p.TAT,  queue_tmp -> p.WT);
		fflush(stdout);
		
		total_burst = total_burst + (queue_tmp -> p.burst);
		mu_tat = mu_tat + (queue_tmp -> p.TAT);
		mu_wt = mu_wt + (queue_tmp -> p.WT);

	}
	cpu_wait = CPU_WAIT;
	mu_tat = mu_tat / p_quantity;
	mu_wt = mu_wt / p_quantity;
	
	printf("\nCPU Wait: %d\n", cpu_wait );
	printf("Processes Finished: %d\n", p_quantity );

	printf("Average Turn-Around Time: %f\n", mu_tat );	

	printf("Average Waiting Time: %f\n", mu_wt );	
	//pthread_mutex_unlock(&mutex);	
	return 0;

}

int runJobScheduler(struct queue *ready_queue, int client_sock){
	
	struct timeval tv;
	gettimeofday(&tv, NULL); 
	
	time_t finish;
	time_t begin;
	
	begin=tv.tv_usec;	
	
	int read_size;	
	int params[3];

	read_size = recv(client_sock , &params , sizeof(params) , 0);

	if(read_size<0){
		return 1;
	}	

	if (read_size == 0) {
		gettimeofday(&tv, NULL); 
		finish = tv.tv_usec;		
		usleep(1000000 - (finish - begin));
		return 2;
	}
	
	
	int burst;
	int priority;
	int arrival;

	if (read_size > 0){
		burst = params[0];
		priority = params[1];
		arrival = params[2];		
		int PID = GLOBAL_PID;
		int reply = htonl(PID);
		write(client_sock , &reply , sizeof(reply));
	}
	

	struct PCB pcb;
	pcb.PID = GLOBAL_PID++;
	pcb.priority = priority;
	pcb.status = 1;	

	struct process proc;
	proc.process_pcb = pcb;
	proc.status = &pcb.status;
	proc.burst = burst;
	proc.curr_burst = burst;
	//pthread_mutex_lock(&mutex);	
	proc.begin_time = SECS_L;
	//pthread_mutex_unlock(&mutex);
	proc.TAT = 0;
	proc.WT = 0;
	

	struct queue *queue_tmp;
	
	queue_tmp = (struct queue *)malloc(sizeof(struct queue));
	//INIT_LIST_HEAD(&queue_tmp->list);
	queue_tmp->p = proc;
	list_add_tail(&(queue_tmp->list), &(ready_queue -> list));	
	printf("Process has arrived at time: %d\n", SECS_L);	
	fflush(stdout);	
	
	gettimeofday(&tv, NULL); 
	finish = tv.tv_usec;		

	usleep(1000000 - (finish - begin));
	SECS_L = SECS_L + arrival;
	
	
	return 0;
};
	
int runCPUScheduler(struct queue *ready_queue, struct queue *done_queue, int algorithm,int quantum){
	
	if (list_empty(&ready_queue -> list) == 0) {
		//pthread_mutex_lock(&mutex);			
		struct list_head *pos;
		struct queue *queue_tmp;
		struct queue *min_p = (list_entry(ready_queue->list.next, struct queue, list)); 
		if (algorithm !=0 && algorithm !=3 ){
	
			list_for_each(pos, &ready_queue -> list){

				queue_tmp = list_entry(pos, struct queue, list);
				
				int pid1= queue_tmp -> p.process_pcb.PID;
				int pid2= min_p -> p.process_pcb.PID;
				
				int burst1= queue_tmp -> p.burst;
				int burst2= min_p -> p.burst;
				
				int priority1= queue_tmp -> p.process_pcb.priority;
				int priority2= min_p -> p.process_pcb.priority;

				int selected_process = select_process(algorithm, pid1, pid2, burst1, burst2, priority1, priority2);
				 
				if (selected_process == 1){
					min_p = queue_tmp;
				}
			}
		}
		
		
		printf("A process has started: ");
		printf("PID: %d | Total Burst: %d | Remaining Burst: %d | Priority: %d\n",min_p->p.process_pcb.PID,  min_p -> p.burst, min_p -> p.curr_burst, min_p -> p.process_pcb.priority);
		fflush(stdout);	
		


		if (algorithm==3){
			if (min_p ->p.curr_burst < quantum){
			
				//pthread_mutex_unlock(&mutex);

				sleep(min_p ->p.curr_burst);
				SECS = SECS + min_p -> p.curr_burst;				
				//pthread_mutex_lock(&mutex);
				min_p -> p.curr_burst = 0;			
			}
			else {
				//pthread_mutex_unlock(&mutex);
				sleep(quantum);
				//pthread_mutex_lock(&mutex);
				
				SECS = SECS + quantum;				
				min_p -> p.curr_burst = (min_p -> p.curr_burst) - quantum;
				list_move_tail(&(min_p->list), &(ready_queue -> list));
			}
		}
		else{
			//pthread_mutex_unlock(&mutex);
			sleep(min_p ->p.burst);
			SECS = SECS + min_p -> p.burst;
			//pthread_mutex_lock(&mutex);
			min_p -> p.curr_burst = 0;
		}
		
		if (min_p -> p.curr_burst == 0){

			min_p -> p.TAT = SECS - (min_p -> p.begin_time);
					
			min_p -> p.WT = (min_p -> p.TAT) - (min_p -> p.burst);
			list_move_tail(&(min_p->list), &(done_queue -> list));
			printf("Process with PID %d finished\n", min_p -> p.process_pcb.PID);
			fflush(stdout);
		}	
		
		//pthread_mutex_unlock(&mutex);

	} else{
		SECS++;
		CPU_WAIT++;
		sleep(1);
	}
 	return 0;

};


int select_process(int algorithm, int pid1, int pid2, int burst1, int burst2, int priority1, int priority2){
	
	if (algorithm== 1){
	
		if (burst1 == burst2){
			if (pid1 < pid2){
				return 1;			
			} else {
				return 0;			
			}	
		}

		else if (burst1 < burst2){			
				return 1;			
		}

		else {
			return 0;			
		}

	}

	if (algorithm == 2){
		if (priority1 == priority2){
			if (pid1 < pid2){
				return 1;			
			} else {
				return 0;			
			}	
		}

		else if (priority1 < priority2){
			return 1;			
		}

		else {
			return 0;			
		}
	
	}

}

int display_help(){
	printf("ARGUMENTS:\n-f: FIRST IN FIRST OUT (FIFO) QUEUE\n-s: SHORTEST JOB FIRST (SJF) QUEUE\n-h: HIGHEST PRIORITY FIRST (HPF) QUEUE\n-r: ROUND ROBIN (RR) QUEUE. QUANTUM AS THIRD PARAMETER. DEFAULT Q = 2\n\nPRESS TAB TO SHOW THE READY QUEUE\nPRESS ESC TO FINISH THE EXECUTION\n");

}


int main(int argc, char **argv)
{
	
	int stop_condition = 0;
	int algorithm;
	int quantum;
	
	if(argc == 1){
		display_help();
		return 0;
	}

	if(strcmp(argv[1], "-f") == 0){
		printf("FIFO Selected\n");	
		algorithm = 0;
	}
	else if(strcmp(argv[1], "-s") == 0){
		printf("SJF Selected\n");	
		algorithm = 1;
	}
	else if(strcmp(argv[1], "-h") == 0){
		printf("HPF Selected\n");		
		algorithm = 2;
	}
	else if(strcmp(argv[1], "-r") == 0){
		
		algorithm = 3;
		quantum = 2;
	} else {
		display_help();
		return 0;
	}
	
	
	if (argc == 3){
		quantum = atoi(argv[2]);
	}
	if (algorithm == 3){
		printf("RR Selected, Q = %d\n",quantum);		
	} 

	struct queue ready_queue;
	INIT_LIST_HEAD(&ready_queue.list);
	struct queue done_queue;
	INIT_LIST_HEAD(&done_queue.list);


	struct thread_args args;

	args.status = &stop_condition;	
	args.ready_queue = &ready_queue;
	args.done_queue = &done_queue;
	args.algorithm = algorithm;
	args.quantum = quantum;


	

		
	pthread_t job_scheduler_thread;
	if(pthread_create(&job_scheduler_thread, NULL, Job_scheduler_thread, &args)){
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	while(list_empty(&ready_queue.list) == 1) {
		
	}	
		
	pthread_t cpu_scheduler_thread;
	if(pthread_create(&cpu_scheduler_thread, NULL, CPU_scheduler_thread, &args)){
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	pthread_t key_reader;
	if(pthread_create(&key_reader, NULL, read_key, &args)){
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	pthread_join(job_scheduler_thread, NULL);
	printf("\nJobScheduler Thread closed\n");
	pthread_join(cpu_scheduler_thread, NULL);
	printf("\nCPUScheduler Thread closed\n");
	pthread_join(key_reader, NULL);
	printf("KeyReader Thread closed\n");

	return 0;



};
