#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <stdbool.h>
#include <ctype.h>

char buf[256];
char buf2[256];
int read_count;
char *ch;
int sockfd;

void *take_input();
void *display_result();

void sighandler(int signo){
	if (signo == SIGINT) {
    write(STDOUT_FILENO, "CTRL-C pressed\n",15);
}	
}
	
int main(int argc, char *argv[]){

	
	int port;
	struct sockaddr_in server_addr;
	struct hostent *host;
	
	if(argc<3){
		perror("argc error");
		write(STDOUT_FILENO,"enter correct arguments\n",24);
		exit(0);
	}


	sockfd= socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd==-1){
		perror("socket error");
	}

	host=gethostbyname(argv[1]);
	if(host==NULL){
		perror("host struct error");
		exit(1);
	}
	bcopy((char *)host->h_addr, (char *)&server_addr.sin_addr.s_addr,host->h_length);
	
	port=atoi(argv[2]);
	server_addr.sin_family= AF_INET;
	server_addr.sin_port=htons(port);
	
	int conn= connect(sockfd, (struct sockaddr *)&server_addr,sizeof(server_addr));
	if(conn==-1){
		perror("connect error");
		exit(1);
	}

	signal(SIGINT, sighandler);

	pthread_t thread1, thread2;
	int  iret1, iret2;

	iret1 = pthread_create( &thread1, NULL, &take_input, NULL);
	if(iret1)
	{
		fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
		exit(EXIT_FAILURE);
	}

	

	iret2 = pthread_create( &thread2, NULL, &display_result, NULL);
	if(iret2)
	{
		fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
		exit(EXIT_FAILURE);
	}
	pthread_join( thread1, NULL);
	pthread_join( thread2, NULL);

	close(sockfd);


	return 0;
}

void *take_input()
{	
	write(STDOUT_FILENO, "List of operations you can perform:\n1)run e.g run gedit\n2)add/sub/mult/div e.g add 2 3 or sub 4 2 etc\n3)kill PID of app e.g kill 1234\n4)list\n5)print *msg*\n6)exit\n",163);
  
	while(true){
	read_count= read(STDIN_FILENO, buf,256);
	//buf[read_count-1]='\0';
	if(read_count==1){
		write(STDOUT_FILENO,"Please enter a command\n",23);
	} 
	else{
	if(read_count<0 ){
		perror("error reading");
		exit(0);
	}
	char temp_buf[256];
	memcpy(temp_buf,buf,256);
	temp_buf[read_count-1]='\0';
	ch=strtok(temp_buf," ");
	if (ch==NULL){
		write(STDOUT_FILENO,"Please enter a command\n",23);
	}
	else{
		if(strcmp(ch,"add")!=0 && strcmp(ch,"sub")!=0 && strcmp(ch,"mult")!=0 && strcmp(ch,"div")!=0 && strcmp(ch,"run")!=0 && strcmp(ch,"list")!=0 && strcmp(ch,"print")!=0 && strcmp(ch,"kill")!=0 && strcmp(ch,"exit")!=0)
	{	
		write(STDOUT_FILENO,"Invalid command,try again\n",26); 	
	}
	
	// if(strcmp(ch,"add")==0 || strcmp(ch,"sub")==0 || strcmp(ch,"mult")==0 || strcmp(ch,"div")==0)
	// {	
		
	//  	int found=0;
	//  	char str[100];

	// 	while((ch= strtok(NULL, " ")) != NULL){
	// 		//printf("ch %s\n",ch );
	// 		//sscanf(ch,"%s",str);
	// 		for(int i=0; i<strlen(ch); i++){
	// 			if(isdigit(ch[i])==0){
	// 				write(STDOUT_FILENO,"Enter only digits",17);
	// 				found=1;
	// 			}
	// 		}
	// 		if(found==0){
	// 			int c=write(sockfd, buf, read_count);
	// 			if (c < 0){
	// 				perror("error writing on socket");
	// 				exit(0);
	// 			}
	// 		}
	// 		else{ 
	// 			break;
	// 		}
	// 	}
	// }
		
	if(strcmp(ch,"exit")==0){
		int c=write(sockfd, buf, read_count);
		int rcount=read(sockfd, buf2,256);
		write(STDOUT_FILENO,buf2,rcount);
		exit(0);
	}
	int c=write(sockfd, buf, read_count);
	if (c < 0){
		perror("error writing on socket");
		exit(0);
	}
	}
	

}
}
}
void *display_result(){
	while(true){
	int rcount=read(sockfd, buf2,256);
	if(rcount<0){
		perror("error reading from socket");
		exit(0);
	}
	if(strcmp(buf2,"Server terminated\n")==0){
		write(STDOUT_FILENO,buf2,rcount);
		exit(0);

	}
	else if(strcmp(buf2,"Msg from server: ")==0){
		write(STDOUT_FILENO,buf2,rcount);
		write(STDOUT_FILENO," ",1);
	}	
	else{
	write(STDOUT_FILENO,buf2,rcount);
	}

}
}