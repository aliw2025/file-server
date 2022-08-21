#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>
#define SIZE 1024
#include <sys/shm.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h> 
#include <time.h>
#include <signal.h>
/* some of the printf lines are commented out
 they can be used for debug purpose only*/

// keep count of files
int file_count=0;
// structure for shared memory of history table
typedef struct file
{
   char key[256];        /* md5 hash of a file*/
   char  time[100];	
   char operation[256];
   char ip[256];
   int count; 		// only the first entry can keep count
   time_t t;

}history;

// structure for keeping track of banned connections
typedef struct ips
{
   /* md5 hash of a file*/
   clock_t time;
   char ip[256];
   // only the first entry can keep count
   int count; 	
	time_t t;	
}  banned;

// pointer to banned struct
banned *ban;
// pointer to history sturct
history *his;

pthread_mutex_t lock;   //lock for critcal section of history
pthread_mutex_t lock1;	// lock for banned connection
// id for his
int id;
// id for ban
int id2;

// used for md5sum
#define STR_VALUE(val) #val
#define STR(name) STR_VALUE(name)
#define PATH_LEN 256
#define MD5_LEN 32



void handle_sigint(int sig) 
{ 
	printf("termination signal Caught  %d\n", sig);
	//detach from shared memory  
        shmdt(his);
	shmdt(ban);	 
        // destroy the shared memory 
        shmctl(id,IPC_RMID,NULL); 
	shmctl(id2,IPC_RMID,NULL); 
 	exit(0);
} 

// the following function is taken from stack overflow
int CalcFileMD5(char *file_name, char *md5_sum)
{
    #define MD5SUM_CMD_FMT "md5sum %." STR(PATH_LEN) "s 2>/dev/null"
    char cmd[PATH_LEN + sizeof (MD5SUM_CMD_FMT)];
    sprintf(cmd, MD5SUM_CMD_FMT, file_name);
    #undef MD5SUM_CMD_FMT
    FILE *p = popen(cmd, "r");
    if (p == NULL) return 0;

    int i, ch;
    for (i = 0; i < MD5_LEN && isxdigit(ch = fgetc(p)); i++) {
        *md5_sum++ = ch;
    }
    *md5_sum = '\0';
    pclose(p);
    return i == MD5_LEN;
}

// function used to check if a path is file or directory
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}
/* to check if passed hash match hash of 
of existing files*/
int  check_hash(char *hash,char *path){
	DIR *dir;	
	struct dirent *ent;
	char md5[MD5_LEN + 1];
    	/*open the directory*/
 	if ((dir = opendir ("./storage")) != NULL) {
  	/* check each file in directory */
 	 while ((ent = readdir (dir)) != NULL) {
		sprintf(path,"./storage/%s",ent->d_name);
		if(is_regular_file(path)){
			// calculate hash of file
			CalcFileMD5(path, md5);
			// if hash matched the passed hash
			if(strcmp(hash,md5)==0){
				printf("match found :%s\n",ent->d_name);
				return 1;
			}
		}
		
    		
  	}
  		closedir (dir);
	} else {
 		 /* could not open directory */
  		perror ("");
  		return -1;
	}
	return 0;
}
/*write file to dir*/
void write_file(FILE *f, int sockfd){
	int n;
  	char buffer[SIZE]={0};
  	while (1) {
		// recieving data from socket
    		n = recv(sockfd, buffer, sizeof(buffer), 0);
    		if (n == 1) {
      			perror("reciev file error");
      			exit(1);
    		}// if revieved done its mean file reading is done
		if(strcmp(buffer,"done")==0){
			break;
		}
		// save the recieved data to a flle
		fprintf(f,"%s",buffer);
   		fflush(f);
    		bzero(buffer, SIZE);
  	}
 
	
}
/*write file to socket*/
void send_file(FILE *fptr1,int sockfd)
{	
	char str[1024];
	int w=0;
	char c;
	// loop until full file is sent
	while(1){
		w=0;
		strcpy(str," ");
		c = fgetc(fptr1); 
	        str[w++]=c;
		// read unitl 1000 char or end of file 
  		while (c!= EOF && w!=1000) 
   		{        // reading a charcter
     	        	c = fgetc(fptr1);
			if(c!=EOF)
			  	str[w++]=c;		 
    		}
		// add null character to terminate string
		str[w++]='\0';
		if(c==EOF){
			
			
			send(sockfd,str,SIZE,0); 
			break;
		}
		//senging the read data to socket
		send(sockfd,str,SIZE,0); 		
		
	}
	// send done when file is completly read
	strcpy(str,"done");
	send(sockfd,str,SIZE,0); 
		
}
/*check if an ip adrees is banned for given time*/
int check_banned(char *ip,int t){
 	// get last index of banned ip list
 	int ind= ban[0].count;
	//printf("count %d \n",ind);
	// loop throung list and fined the ip 
	for(int i=0;i<ind;i++){
		//printf("count %d ip: %s   ip2:%s\n",ind,ban[i].ip,ip);
		// if ip is found check if its still have to wait
		if(strcmp(ban[i].ip,ip)==0){
			// calculate differece of current time and time of ban of ip
			double time_spent = time(NULL)-ban[i].t;
			printf("time spent :%f\n",time_spent);	
			if(time_spent>t){
				
				//printf("time spent :%f\n",time_spent);
				// return 0 mean it is not banned
				return 0;
			}
			else // return 1 mean it is still banned
				return 1;
				
		}
	}
// return 0 if ip is not found means it was never banned
return 0;
}
// main function
int main(int c,char *argv[]){
	// check if arugments are not provided
	if(c<3){
		printf("usage: k t1 t2\n");
		return -1;
	}
	// converting the arguments from string to int
	int k=atoi(argv[1]);
	int t=atoi(argv[2]);
	int t2=atoi(argv[3]);
	//storing  ip adress of the server
	char *ip = "127.0.0.1";
	// port number 
	int port  = 8888;
	/* registring a signal handler for ctrl-c signal
	so when it is recieved all shard memory are cleared*/
	signal(SIGINT, handle_sigint); 
	// creating unique keys for shared memories for history and banned list
	key_t key = ftok("aaaaa", 1);
	key_t key2=ftok("bbbbb", 2);
	
	// shared memory for history
	id = shmget(key, 2560 * sizeof(his), IPC_CREAT | 0644);
	his = (history *) shmat(id, NULL, 0);
	// shared memory for banned list
	id2=shmget(12222, 2560 * sizeof(ban), IPC_CREAT | 0644);
	ban = (banned *) shmat(id2, NULL, 0);
	
	// initalizing the count of element in both memories
	his[0].count=0;
	ban[0].count=0;	

	// variables for creatign server socket
	int e;
	int sockfd, new_sock;
	struct sockaddr_in server_addr, new_addr;
	socklen_t addr_size;
	char buffer[SIZE];
	pid_t childpid;
	FILE *f;
	char filepath[1024];

	// 1. creating the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
    		perror("[-]Error in socket");
    		exit(1);
	}
	
	printf("[+]Server socket created.\n");
	// 2. writing the data in the structure
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = port;

	// 3. binding the ip address with the port
	addr_size = sizeof(server_addr);
	e = bind(sockfd, (struct sockaddr*)&server_addr, addr_size);
	if (e < 0){
		perror("[-]Error in bind");
 		exit(1);
	}
	printf("[+]Binding successfull.\n");
	// 4. listening to the clients
	e = listen(sockfd, 10);
	if (e < 0) {
		perror("[-]Error in listen");
		exit(1);
	}
	printf("[+]Listening...\n");

	// 5. accepting the client connection.
	while(1) {  
    		addr_size = sizeof(new_addr);
    		new_sock = accept(sockfd, (struct sockaddr*)&new_addr, &addr_size);
    		if (new_sock < 0){
      			perror("[-]Error in accpet");
      			exit(1);
    		}
		printf("checking connection validity\n");
		/* check if the accepted connection is in banned connections 
		 and if its still banned*/
		if(!check_banned(inet_ntoa(new_addr.sin_addr),t))
			// if its not banned accept the connection and continue
			printf("Connection accepted from %s:%d\n", inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));
		else {
			// if its still banned 
			printf("connection dropped : wait.....\n");
			close(new_sock);
			// continue in loop and accept next connection
			continue;
		}
		// creating child process
		childpid = fork();
		// only child process can handle the connectin,
		// parent process only accept new process
    		if (childpid == 0){
      			close(sockfd);
		// loop until connectino clossed
      		while(1){
			
			printf("Ready to take next Command:\n");
			struct timeval timeout;
			fd_set read_fd_set;
			FD_ZERO(&read_fd_set);
			FD_SET((unsigned int)new_sock, &read_fd_set);
			timeout.tv_sec = t2;
			timeout.tv_usec = 0;
			/* wait for client to send data ,if t2 secs are passed
			dropped the connection*/
			select(new_sock+1, &read_fd_set, NULL, NULL, &timeout);
			if (!(FD_ISSET(new_sock, &read_fd_set))) {
				
				printf("client inactive for %d secs closing the connection\n",t2);       			
				shutdown(new_sock, SHUT_RDWR);
				close(new_sock);
				exit(0);
			
    			}
			// recieve command from client
        		recv(new_sock, buffer, SIZE, 0);
			printf("recv command : %s\n",buffer);
			// decodinng the command
        		if (strstr(buffer, "STORE") !=NULL ){
        			// creating path for storage directory
				sprintf(filepath,"storage/file_%d.txt",file_count);
				file_count++;
				//printf("!%s\n",filepath);
          			//opening a file to store the recieving file";			
				f = fopen(filepath, "w+");
  				if (f == NULL) {
   					 perror("[-]Error in creating file");
    				 	 exit(1);
  				}
				// store the recieving file
				write_file(f,new_sock);
       		 		printf("file received\n");
				fclose(f);
				// vairable for hash
    				char md5[MD5_LEN + 1];
				// calculating hash of file
    				if (!CalcFileMD5(filepath, md5)) {
        				puts("Error occured in hasing!");
    				} else {
        				printf("Success! MD5 sum is: %s\n", md5);
					strcpy(buffer,md5);
					send(new_sock,buffer,SIZE,0);
    				}
				//locking the shared memory for writing
				pthread_mutex_lock(&lock); 
				// current index of history table
				int in=his[0].count;
				strcpy(his[in].key,md5);
				strcpy(his[in].operation,"STORE");
				strcpy(his[in].ip,inet_ntoa(new_addr.sin_addr));			
				// time			
				time_t mytime = time(NULL);
    				char * time_str = ctime(&mytime);
        			time_str[strlen(time_str)-1] = '\0';
				strcpy(his[in].time,time_str);
        			//printf("Current Time : %s\n", time_str);
				//printf("size: %d\n",his[0].count);			  
				his[0].count++;
				pthread_mutex_unlock(&lock);
				// history updated
        	}else if (strstr(buffer, "GET") !=NULL){
          		// if get command is recieved tokenize command to extracct the key
			char s[1000];
			char tokens[10][256];
			strcpy(s, buffer);
			char* token = strtok(s, " ");
			int i=0;
			while (token) {
				strcpy(tokens[i++],token);
    				//printf("token: %s\n", token);
    				token = strtok(NULL, " ");
			}
			int ret=check_hash(tokens[1],filepath);	
			if(ret==1){
				// if file found with marching hash
				printf("file found\n");
				//telling client that  he is good to revieve file
				strcpy(buffer,"done");
				send(new_sock,buffer,SIZE,0);
				// opening the file to be sent
				FILE *fp=fopen(filepath,"r"); 
          			send_file(fp,new_sock);
				// recording history
				// locking the critcial section
				pthread_mutex_lock(&lock);
				int in=his[0].count;
				strcpy(his[in].key,tokens[1]);
				strcpy(his[in].operation,"GET");
				strcpy(his[in].ip,inet_ntoa(new_addr.sin_addr));			
				// time			
				time_t mytime = time(NULL);
    				char * time_str = ctime(&mytime);
        			time_str[strlen(time_str)-1] = '\0';
				strcpy(his[in].time,time_str);
				his[in].t=mytime;
        			//printf("Current Time : %s\n", time_str);
				//printf("size: %d\n",his[0].count);			  
				his[0].count++;
				pthread_mutex_unlock(&lock);

			}else{	
				// if file not found
				printf("file not found\n");
				// notifying the client that key is wrong
				strcpy(buffer,"key not found");
				send(new_sock,buffer,SIZE,0);
				// decrementing the wrong key atemts left
				k--;
				if(k==0){  // if no mor attempts are left
					// adding the client to banned list
					clock_t begin = clock();
					// locking
					pthread_mutex_lock(&lock1);
					//printf("lock aquired\n");
					int ind=ban[0].count;
					ban[ind].time=begin;	
					ban[ind].t=time(NULL);			      
					strcpy(ban[ind].ip,inet_ntoa(new_addr.sin_addr));
					ban[0].count++;		
					//printf("check: %d",ban[0].count);
					// closing the client			
					shutdown(new_sock, SHUT_RDWR);
					close(new_sock);
					pthread_mutex_unlock(&lock1);
					//printf("lock released\n");
					exit(0);
				}					
			}
        	}else if (strstr(buffer, "DELETE") !=NULL){
          		
			char s[1000];
			char tokens[10][256];
			strcpy(s, buffer);
			char* token = strtok(s, " ");
			int i=0;
			while (token) {
				strcpy(tokens[i++],token);
    				printf("token: %s\n", token);
    				token = strtok(NULL, " ");
			}
			int ret=check_hash(tokens[1],filepath);	
			if(ret==1){
				printf("file found\n");
				strcpy(buffer,"done");
				send(new_sock,buffer,SIZE,0);
				remove(filepath);	
				pthread_mutex_lock(&lock);			
				int in=his[0].count;
				strcpy(his[in].key,tokens[1]);
				strcpy(his[in].operation,"DELETE");
				strcpy(his[in].ip,inet_ntoa(new_addr.sin_addr));			
				// time			
				time_t mytime = time(NULL);
    				char * time_str = ctime(&mytime);
        			time_str[strlen(time_str)-1] = '\0';
				strcpy(his[in].time,time_str);
        			printf("Current Time : %s\n", time_str);
				printf("size: %d\n",his[0].count);			  
				his[0].count++;
				pthread_mutex_unlock(&lock);
			}else{
				printf("file not found\n");
				strcpy(buffer,"key not found");
				send(new_sock,buffer,SIZE,0);
				k--;
				if(k==0){
					clock_t begin = clock();
					pthread_mutex_lock(&lock1);
					//printf("lock aquired\n");
					int ind=ban[0].count;
					ban[ind].time=begin;	
					ban[ind].t=time(NULL);			      
					strcpy(ban[ind].ip,inet_ntoa(new_addr.sin_addr));
					ban[0].count++;					
					shutdown(new_sock, SHUT_RDWR);
					close(new_sock);
					pthread_mutex_unlock(&lock1);
					exit(0);
				}
				
					
			}
        	}else if (strstr(buffer, "HISTORY") !=NULL){
          		
			char s[1000];
			char tokens[10][256];
			strcpy(s, buffer);
			char* token = strtok(s, " ");
			int i=0;
			while (token) {
				strcpy(tokens[i++],token);
    				//printf("token: %s\n", token);
    				token = strtok(NULL, " ");
			}
			int in=his[0].count;
			int flag=0;	
			char newbuff[2024];
			// go throung the history list and send every entry 
			// matcing the passed key
			for(int i=0;i<in;i++){
				//printf("jere %s %s\n",his[i].key,tokens[1]);
				// send data of matching key
				if(strcmp(his[i].key,tokens[1])==0){
					flag=1;
					//printf("mathceddddd\n");
					// storing all the info in buffer
					sprintf(buffer,"keY:%s  operation:%s  ip:%s  time:%s",his[i].key ,
					his[i].operation , his[i].ip,his[i].time);
					// sending data
					send(new_sock,buffer,SIZE,0);					
				}
				
			}	
			if(flag==1){
				// if at  least  one key is matched
				// notify client that history sent sucess
				strcpy(buffer,"done");
				send(new_sock,buffer,SIZE,0);
				
			}else{
				// if no matching key 
				// notify error 
				strcpy(buffer,"HISTORY ERROR");
				send(new_sock,buffer,SIZE,0);
				k--;
				if(k==0){
					clock_t begin = clock();
					pthread_mutex_lock(&lock1);
					int ind=ban[0].count;
					ban[ind].time=begin;	
					ban[ind].t=time(NULL);			      
					strcpy(ban[ind].ip,inet_ntoa(new_addr.sin_addr));
					ban[0].count++;					
					shutdown(new_sock, SHUT_RDWR);
					close(new_sock);
					pthread_mutex_unlock(&lock1);
					exit(0);
				}
				printf("history not found\n");
					
			}
        	}
        	else if (strstr(buffer, "QUIT") !=NULL){
          		// connection disconnected.
			shutdown(new_sock, SHUT_RDWR);
			close(new_sock);
          		printf("Connection disconnected from %s:%d\n", inet_ntoa(new_addr.sin_addr), ntohs(new_addr.sin_port));
          		exit(1);
        	}
        	bzero(buffer,SIZE);
		
   
    	}

      
    }

  }

}


