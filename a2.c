#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <stdlib.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int sem_id;
int th_no;
pthread_mutex_t lock;
pthread_cond_t cond;

void P (int sem_id, int sem_no){

    struct sembuf op = {sem_no, -1, 0};

    semop(sem_id, &op, 1);
}

void V (int sem_id, int sem_no){

    struct sembuf op = {sem_no, +1, 0};

    semop(sem_id, &op, 1);
}

//for P4
void* create4Threads (void* arg){

    int th_id = *((int*) arg);
    //T4.2 asks for permission to start
    if(th_id == 2)
    	P(sem_id, 3);
    //asks for permission to start before T4.3
    if (th_id == 3) 
    	P(sem_id, 0);
    info(BEGIN, 4, th_id);
    //T4.4 receives the permission to start before T4.3
    if (th_id == 4) 
    	V(sem_id, 0);
    //asks for permission to end before T4.4
    if (th_id == 4) 
    	P(sem_id, 1);
    info(END, 4, th_id);
    //T4.4 receives the permission to end as T4.3 ended
     if (th_id == 3) 
     	V(sem_id, 1);
     //T5.1 receives the permission to start as T4.2 ended
     if (th_id == 2)
     	V(sem_id, 4);

    return NULL;
}


//for P6
void* create38Threads (void* arg){
 
    int th_id = *((int*) arg);
 
    //********ENTRANCE OF THE CRITICAL REGION********
 
    //checks if the entrance is allowed
    if (pthread_mutex_lock(&lock) != 0) {
        perror("Cannot take the lock");
        exit(1);
    }
 
    //if there are already 5 threads running simultaneously,
    //then the threads that want to enter in the critical
    //region will wait until it receives the signal that
    //a new place is available
    while (th_no >= 5) {
        if (pthread_cond_wait(&cond, &lock) != 0) {
            perror("Cannot wait for condition");
            exit(2);
        }
    }
 
    //th_no counts how many threads are in the critical region
    th_no++;
    if (pthread_mutex_unlock(&lock) != 0) {
        perror("Cannot release the lock");
        exit(3);
    }
 
    //********BEGINNING of the CRITICAL REGION********
   
    //a thread must be running here only when it is safe,
    //meaning that th_no should never exceed 5 at any time
    info(BEGIN, 6, th_id);
    //********END of the CRITICAL REGION********
 
   
    //********EXIT of the CRITICAL REGION********
   
    //checks whether or not the exit from the critical region is allowed
    if (pthread_mutex_lock(&lock) != 0) {
        perror("Cannot take the lock");
        exit(4);
    }
 
    //th_no is decremented as a thread left the critical region
    th_no--;
 
    info(END, 6, th_id);
 
    //signals that there's a new space in the critica region for a new thread
    if (pthread_cond_signal(&cond) != 0) {
        perror("Cannot signal the condition waiters");
        exit(5);
    }
 
    if (pthread_mutex_unlock(&lock) != 0) {
        perror("Cannot release the lock");
        exit(6);
    }
 
    return NULL;
}

//for P5
void* create6Threads (void* arg){

    int th_id = *((int*) arg);
    //T5.1 asks for permission to start
    if (th_id == 1)
    	P(sem_id, 4);
    info(BEGIN, 5, th_id);
    info(END, 5, th_id); 
    //T4.2 receives the permission to start as T5.5 ended
    if (th_id == 5)
    	V(sem_id, 3);		
    return NULL;
}


int main(){
    init();
    sem_id = semget(IPC_PRIVATE, 5, IPC_CREAT | 0600);
    info(BEGIN, 1, 0);
	int P2 = fork();

     //P2 was successfully created
    if (P2 == 0){
    	info(BEGIN, 2, 0);
    	int childP7 = fork();
    	//the child process P7 was successfully created
    	if (childP7 == 0){
    		info(BEGIN, 7, 0);
    		info(END, 7, 0);

    	} else if (childP7 > 0){ //the parent process of P7 -> P2
    		waitpid(childP7, NULL, 0);
    		info(END, 2, 0);
    	}

    } else if (P2 > 0){ //the parent process of P7 -> P1
    	int P3 = fork();
    	//the child process P3 was successfully created
    	if(P3 == 0){
    		info(BEGIN, 3, 0);
    		info(END, 3, 0);
    	} else if (P3 > 0){ //the parent process of P3 -> P1
    		int P4 = fork();
    		//the child process P4 was successfully created
    		if(P4 == 0){
    			info(BEGIN, 4, 0); 

    				pthread_t t[4];
    				int th_arg[4];
    				semctl(sem_id, 0, SETVAL, 0);
    				semctl(sem_id, 1, SETVAL, 0);
    				//create 4 threads with {1,2,3,4} as ids
    			 	for (int i=1; i <= 4; i++) { 
    			 		th_arg[i] = i;
    					pthread_create(&t[i], NULL, create4Threads, &th_arg[i]);
    				}
    				for (int i=1; i <= 4; i++) 
    					pthread_join(t[i], NULL);

    			info(END, 4, 0);
    		} else if (P4 > 0){ //the parent process of P4 -> P1
    			int P5 = fork();
    			//the child process P5 was successfully created
    			if (P5 == 0){
    				info(BEGIN, 5, 0);
						pthread_t th[6];
    					int t_arg[6];
    					semctl(sem_id, 3, SETVAL, 0);
						semctl(sem_id, 4, SETVAL, 0); 
    					//create 6 threads with {1,2,3,4,5,6} as ids
    					 for (int i=1; i <= 6; i++) { 
    					 	t_arg[i] = i;
    						pthread_create(&th[i], NULL, create6Threads, &t_arg[i]);
    					}
    					for (int i=1; i <= 6; i++) 
    						pthread_join(th[i], NULL);

    				info(END, 5, 0);
    			} else if (P5 > 0){//the parent process of P5 -> P1
    				int P6 = fork();
    				//the child process P6 was successfully created
    				if (P6 == 0){
    					info(BEGIN, 6, 0);
    						pthread_t th[38];
    						int t_arg[38];
    						//create 38 threads with {1,2,3,4,...,38} as ids
    					 	for (int i=1; i <= 38; i++) { 
    					 		t_arg[i] = i;
    							pthread_create(&th[i], NULL, create38Threads, &t_arg[i]);
    						}
    						for (int i=1; i <= 38; i++) 
    							pthread_join(th[i], NULL);

    					info(END, 6, 0);
    				} else if (P6 > 1){ //the parent process of P6 -> P1
    					//the parent process waits for all its child processes to end
    					waitpid(P2, NULL, 0);
    					waitpid(P3, NULL, 0);
    					waitpid(P4, NULL, 0);
    					waitpid(P5, NULL, 0);
    					waitpid(P6, NULL, 0);
   						info(END, 1, 0);

    				}
    			}
    		}
    	}
    }
    return 0;
}
