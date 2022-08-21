#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <string.h>
int indexx=0; 		//common index for writing and reading page
int sum=0;		// variable to store the sum
pthread_mutex_t lock;   //lock for critcal section
char book[20];		// book for writng pages
 FILE *fptr1, *fptr2; 
int done=9;
//semophore :initalized with 20 ,
// writer decrements it 20 times(i.e write 20 entries) 
//then wait untill reader reads it and increment it.   
sem_t empty;	

//semaphore : initialized with 0
// the reader waits for this to be incremented by writer. if 
// a page is written other wise waits	
sem_t full;		

/*
-- read function
-- read the value written in book
-- return nothing
*/

void *reader(){



    fptr2 = fopen("out.txt", "w"); 
    if (fptr2 == NULL) 
    { 
        printf("Cannot open file "); 
        exit(0); 
    } 
// ftok to generate unique key 
     key_t key = ftok("aaaaa", 1);
  
    // shmget returns an identifier in shmid 
    int shmid = shmget(key,1024,0666|IPC_CREAT); 
  
    // shmat to attach to shared memory 
    char *str = (char*) shmat(shmid,(void*)0,0); 
  
    

	while(1){
		//wait for at least 1 page is written
		
        	sem_wait(&full);
		//locking the critical section
		pthread_mutex_lock(&lock); 
		//summing the value
		sleep(1);
		//book[indexx]= ' ';
		indexx--;
		//unlocking the critical section
		fwrite(str,1,strlen(str),fptr2); 
		pthread_mutex_unlock(&lock);
		//printf("page read remaining:%d\n",indexx);
		//printf("Data read from memory: %s\n",str); 
		//tell writer that reader have read one page. 
        	sem_post(&empty);
		if(done==1){
			pthread_mutex_unlock(&lock);
			sem_post(&empty);
			break;
		}
		  

	} 
	 //detach from shared memory  
         shmdt(str); 
         // destroy the shared memory 
        shmctl(shmid,IPC_RMID,NULL); 
}

/*
-- write  function
--  write page to book
-- return nothing
*/

void *writer(){
  
  char c;

    fptr1 = fopen("input.txt", "r"); 
    if (fptr1 == NULL) 
    { 
        printf("Cannot open file "); 
        exit(0); 
    } 
  
   
     key_t key = ftok("aaaaa", 1);
  
    // shmget returns an identifier in shmid 
    int shmid = shmget(key,1024,0666|IPC_CREAT); 
  
    // shmat to attach to shared memory 
    char *str = (char*) shmat(shmid,(void*)0,0); 
  
      
    //detach from shared memory  
    
    int w=0;
	while(1){
		//wait after twenty pages are written.
		sem_wait(&empty);
		printf("1\n");
		//locking the critical section
		pthread_mutex_lock(&lock); 	
		//summing the value
		sleep(1);
		 w=0;
		strcpy(str," ");
   		c = fgetc(fptr1); 
	        str[w++]=c;
  		while (c!= EOF && w!=1000) 
   		{ 
    		        
     	        	c = fgetc(fptr1);
			if(c!=EOF)
			 	str[w++]=c;		 
    		} 
		if(c==EOF){
			printf("done\n");
			str[w++]='\0';	
			pthread_mutex_unlock(&lock);
			printf(" written %s by thread: %ld\n",str, pthread_self());
			//tell reader that a page has been written		
			sem_post(&full);
			done=1;
			break;		
	
		}
		if(w==0){
		 w=0;
		}
		//book[indexx]='p';
		
		indexx++;
		//unlocking the critical section
		pthread_mutex_unlock(&lock);
		printf(" written %s by thread: %ld\n",str, pthread_self());
		//printf(" written %d by thread: %ld\n",indexx, pthread_self());
		//tell reader that a page has been written		
		sem_post(&full);
				
	}  
	    shmdt(str); 
		        shmctl(shmid,IPC_RMID,NULL); 
}
/*
-- main  function
-- recieve no arguments
-- take user input for reader and writer threads
-- create reader and writer threads
-- return 0 on sucess -1 on faliure
*/
int main(){
int r=1;
int w=1;

	
	
pthread_t id1[w];
pthread_t id2[r];
	//creating the lock for matual exclusion of threads
	if (pthread_mutex_init(&lock, NULL) != 0) { 
        	printf("\n mutex init has failed\n"); 
        	return 1; 
        } 
	sem_init(&empty,0,1);
        sem_init(&full,0,0);
	//creating thread for writer
	for(int i=0;i<w;i++){
		
		pthread_create(&id1[i],NULL,writer,NULL);

	}
	//creating thread for reader 
	for(int i=0;i<r;i++){
		
		pthread_create(&id2[i],NULL,reader,NULL);

	}
	for(int i=0;i<w;i++){
		
		pthread_join(id1[i],NULL);

	}
	for(int i=0;i<r;i++){
		
		pthread_join(id2[i],NULL);

	}




return 0;
}


