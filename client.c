#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define SIZE 1024



// function to write file to directory
void write_file(FILE *f, int sockfd){
	int n;
  	char buffer[SIZE]={0};
  	while (1) {
    		n = recv(sockfd, buffer, sizeof(buffer), 0);
    		if (n == 1) {
      			perror("reciev file error");
      			exit(1);
    		}
		if(strcmp(buffer,"done")==0){
			break;
		}
		fprintf(f,"%s",buffer);
   		fflush(f);
    		bzero(buffer, SIZE);
  	}	
}

// read file and send  to socket
void send_file(FILE *fptr1,int sockfd)
{	char str[1024];	    
	int w=0;
	char c;
	while(1){
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
		str[w++]='\0';
		if(c==EOF){
			
			
			send(sockfd,str,SIZE,0); 
			break;
		}
		send(sockfd,str,SIZE,0); 		
		
	}
	
	strcpy(str,"done");
	send(sockfd,str,SIZE,0); 
		
}
int main(){
	// ip address and port
  	char *ip = "127.0.0.1";
  	int port = 8888;
   	FILE *fp;
  	// variables and structures,
  	int e;
  	int sockfd;
  	struct sockaddr_in server_addr;
  	char buffer[SIZE];
  	char file_path[1000];

  	// 1. creating the socket
  	sockfd = socket(AF_INET, SOCK_STREAM, 0);
  	if(sockfd < 0){
    		perror("[-]Error in socket");
    		exit(1);
  	}
  	printf("[+]Client socket created.\n");

  	// 2. writing the data in the structure
  	server_addr.sin_family = AF_INET;
  	server_addr.sin_addr.s_addr = inet_addr(ip);
  	server_addr.sin_port = port;

 	 // 3. connect to the server
  	e = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
  	if(sockfd < 0){
    		perror("[-]Error in connect");
    		exit(1);
  	}
	// promting user the options	
  	printf("[+]Connected to the server\n");
  	printf("\n");
  	printf("LIST of the commands.\n");
  	printf("STORE file name.\n");
  	printf("GET key_file.\n");
  	printf("DELETE key_file \n");
  	printf("History key_file \n");
  	printf("QUIT - disconnect from the server.\n");
  	char *file;
  	while(1){
		
    		bzero(buffer,SIZE);
    		fflush(stdout);
    		printf("> ");
    		fgets(buffer, SIZE, stdin);
    		printf("%s",buffer);
		char s[1000];
		char tokens[10][256];
		int tok_count=0;
		strcpy(s, buffer);
		s[strlen(s)-1]='\0';
		char* token = strtok(s, " ");
		int i=0;
		// tokkenizing the command
		while (token) {
			strcpy(tokens[i++],token);
    			//printf("token: %s\n", token);
    			token = strtok(NULL, " ");
			tok_count++;
		}	
		if (strcmp(tokens[0], "STORE") == 0) {
        		if (tokens[1] == NULL) {
			          			
				printf("[-]Specify the correct filename.\n");
	
        		}else{	
          			// creating the file path of be oppend and sent
				// it will work even if file name have spaces
				for(int j=1;j<i;j++){
					//int ret=sprintf(file_path,"%s %s",file_path,tokens[j]);
					strcat(file_path,tokens[j]);
					if(j!=i-1)
					strcat(file_path," ");
				}
				
				printf("file path :%s\n",file_path);
          			fp=fopen(file_path,"r");
				printf("sending file\n");
				// sending command to srver
				strcpy(buffer,"STORE");
				send(sockfd,buffer,SIZE,0);         
          			send_file(fp,sockfd);
				strcpy(file_path,"");
				// revieving key from server
				recv(sockfd, buffer, SIZE, 0);
				printf("key: %s\n",buffer);	
				printf("[+] data file send succefullly\n");				   
        		}
      		}else if(strcmp(tokens[0], "GET") == 0) {
        		
        		if (strcmp(tokens[1],"") == 0) {
			          			
				printf("[-]Specify the  key.\n");
	
        		}else if (strcmp(tokens[2],"") == 0) {
			          			
				printf("[-]Specify the  filename.\n");

			}
        		else{	
					
          			sprintf(buffer,"%s %s",tokens[0],tokens[1]);
				printf("command:%s\n",buffer);
          			fp=fopen(tokens[2],"w");
				if(fp==NULL){
					
					printf("error %s: file name not provided\n",tokens[2]);
					continue;
				}
				send(sockfd,buffer,SIZE,0);
				recv(sockfd, buffer, SIZE, 0);
				if(strcmp(buffer,"done")==0){
					write_file(fp,sockfd);
					printf("[+]  file recieved succefullly\n");
				}         
				else{
					
					printf("[+]  key was wrong, please enter a valid key\n");
				
				}          			
				
      			}
			
    		}else if(strcmp(tokens[0], "DELETE") == 0) {
        		
        		if (strcmp(tokens[1],"") == 0){
			          			
				printf("[-]Specify the correct key.\n");
	
        		}else{		
          			sprintf(buffer,"%s %s",tokens[0],tokens[1]);
				printf("file:%s",buffer);
				send(sockfd,buffer,SIZE,0);
				recv(sockfd, buffer, SIZE, 0);
				if(strcmp(buffer,"done")==0){
					
					printf("[+]  file deleted succefullly\n");
				}         
				else{
					
					printf("[+]  key was wrong, please enter a valid key\n");
				
				}          			
				
      			}
		}else if(strcmp(tokens[0], "HISTORY") == 0) {
        		
        		if (strcmp(tokens[1],"") == 0) {
			          			
				printf("[-]Specify the correct key.\n");
	
        		}else{		
          			sprintf(buffer,"%s %s",tokens[0],tokens[1]);
				send(sockfd,buffer,SIZE,0);
					while (1) {
    						int n = recv(sockfd, buffer, sizeof(buffer), 0);
    						if (n == 1) {
      							perror("reciev file error");
      						exit(1);
    						}
						if(strcmp(buffer,"HISTORY ERROR")==0){
							printf("%s\n",buffer);							
							break;
						}else if(strcmp(buffer,"done")==0){
							break;		
						}	
						printf("%s\n",buffer);
    						bzero(buffer, SIZE);
					}	
      			}
		}
		else if(strcmp(tokens[0], "QUIT") == 0) {
        		// disconnect from the server.
        		printf("[+]Disconnected from the server.\n");
        		send(sockfd, tokens[0], SIZE, 0);
        		break;
      		}else {
       			
			printf("[-]Invalid command\n");

      		}

		// clearing the bufffer
		for(int i=0;i<tok_count;i++){
			
			strcpy(tokens[i],"");

		}
    		bzero(buffer, SIZE);
    		bzero(buffer,SIZE);
		strcpy(s, "");
		token=NULL;
		
  }

}
