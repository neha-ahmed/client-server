#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <wait.h> 
#include <fcntl.h>
#include <pthread.h>
 #include <arpa/inet.h>

char *ch;
int client_sockfd;
char buf[1024];
char buf2[1024];
char buf3[1024];
int rval;
int pid;
int pipefd[2];
int count;
int tid;
pthread_t thread1, thread2, thread3;

void *takeClientInput();
void *takeInput();
void *handleServerInput(void *p);
void *writeList(void *p);
void *readInput();
void serverInputfunc(int clientreadpipe, int clientfd, int serverwritepipe);
void add();
void sub();
void multiply();
void divide();
void run();
void list();
void print();
void Kill();
void Exit();

typedef struct table
{
	int childPID;
	int parentPID;
	char name[100];
	time_t start_time;
	time_t end_time;
	int elapsed_time;
	int activeStatus;

}table;

table t1[100];
int listSize=0;

//client list maintained at connection handler
typedef struct table2
{
	int clientfd;
	int clientpid;
	int clientread;
	int clientwrite;
	int serverread;
	int serverwrite;
}table2;

table2 clientlist[100];
int clistsize=0;

void sighandler(int signo) {
	if (signo == SIGTERM) {
    write(STDOUT_FILENO, "Process gracefully terminated\n", 31);
}
    if (signo == SIGINT) {
    write(STDOUT_FILENO, "CTRL-C pressed\n",15);
 //    write(client_sockfd,"Server terminated\n",18);
	// //kill(0, SIGKILL);
 //    exit(0);
}
       if (signo == SIGCHLD) {
       	//int status;
       	int pid= wait(NULL);
       	char p[10];
       	write(STDOUT_FILENO,"process killed: ",16);
       	int c=sprintf(p,"%d\n",pid);
       	write(STDOUT_FILENO,p,c);
    	int count=0; 
		for (int i = 0; i < listSize; ++i)
			{   if (pid==t1[i].childPID)
				{    
					count++;
					if (t1[i].activeStatus==1)
					{
						t1[i].activeStatus=0;
						t1[i].end_time=time(NULL);
						t1[i].elapsed_time= t1[i].end_time - t1[i].start_time;
						write(STDOUT_FILENO,"Process killed externally.\n",27);
						//write(client_sockfd,"Process killed externally.\n",27);
						break;
					}
					
				}
			}
}
return;
}

int main(void){
	pipe(pipefd);
	int sockfd;
	char buf[1024];
	
	
	int  tret1, tret2, tret3;

	tid=pthread_self();          //main thread
	struct sockaddr_in server_addr;

	sockfd=socket(AF_INET, SOCK_STREAM,0);
	if(sockfd==-1){
		perror("socket error");
		exit(0);
	}

	server_addr.sin_family= AF_INET;
	server_addr.sin_port=0;
	server_addr.sin_addr.s_addr= INADDR_ANY;

	if(bind(sockfd,(struct sockaddr *)&server_addr, sizeof(server_addr))==-1){
		perror("bind error");
		exit(1);
	}

	int length = sizeof(server_addr);
	if (getsockname(sockfd, (struct sockaddr *) &server_addr, (socklen_t*) &length)) {
		perror("getting socket name error");
		exit(1);
	}
	int port=ntohs(server_addr.sin_port);
	char ports[10];
	int cport= sprintf(ports,"%d\n",port);
	write(STDOUT_FILENO,"Port number:",12);
	write(STDOUT_FILENO,ports,cport);
	

	if(listen(sockfd,10)==-1){    //making queue for 10 incoming connections
		perror("listen error");
	}

	//signal(SIGTERM, sighandler);
	signal(SIGCHLD, sighandler);
	signal(SIGINT, sighandler);

	do{
		client_sockfd=accept(sockfd,0,0);   //CONNECTION HANDLER
		if(client_sockfd==-1){
			perror("accept error");
		}

		int clientpipe[2]; 
	 	int pipeserver[2];
		pipe(clientpipe);	//pipe from CH to conn
		pipe(pipeserver);  //pipe from server to CH

		int arr2[1];
		arr2[0]=pipeserver[0];
		tret1 = pthread_create( &thread1, NULL, takeInput,(void *) arr2 );    //conn thread to take INPUT 
		if(tret1)
			{
				fprintf(stderr,"Error - pthread_create() return code: %d\n",tret1);
				exit(EXIT_FAILURE);
			}

		pid=fork();      
		if(pid==-1){
			perror("fork error");
		}
		if (pid==0){   //CLIENT HANDLERS

			//close(sockfd);
			tret2 = pthread_create( &thread2, NULL, takeClientInput, NULL);          //CH thread to work on client commands
			if(tret2)
			{
				fprintf(stderr,"Error - pthread_create() return code: %d\n",tret2);
				exit(EXIT_FAILURE);
			}
			int arr[3];
			arr[0]=clientpipe[0];
			arr[1]=client_sockfd;
			arr[2]=pipeserver[1];

			tret3 = pthread_create(&thread3, NULL, handleServerInput,(void *)arr);   //CH thread to work on server commands
			if(tret3)
			{
				fprintf(stderr,"Error - pthread_create() return code: %d\n",tret3);
				exit(EXIT_FAILURE);
			}
			pthread_join( thread1, NULL);
			//pthread_join( thread2, NULL);
			pthread_join( thread3, NULL);
					
		}
		char pidclient[10];
		int s=sprintf(pidclient, "%d\n", pid);
		write(STDOUT_FILENO,"Pid of client connected:",24);
		write(STDOUT_FILENO,pidclient,s);

		clientlist[clistsize].clientfd= client_sockfd;
		clientlist[clistsize].clientpid= pid;
		clientlist[clistsize].clientread=clientpipe[0];
		clientlist[clistsize].clientwrite=clientpipe[1];
		clientlist[clistsize].serverread=pipeserver[0];
		clientlist[clistsize].serverwrite=pipeserver[1];
		clistsize++;
		
		close(client_sockfd);
		

	} while (1);
	
	close(sockfd);

	pthread_exit(NULL);
}


void *takeInput(void * p){

	int * arrc=(int*)p;
	pthread_join( thread2, NULL);
	pthread_t thread4,thread5;

	int t1 = pthread_create( &thread4, NULL, readInput, NULL);    
		if(t1)
			{
				fprintf(stderr,"Error - pthread_create() return code: %d\n",t1);
				exit(EXIT_FAILURE);
			}

	
	int t2 = pthread_create( &thread5, NULL, writeList, (void *)arrc);    
		if(t2)
			{
				fprintf(stderr,"Error - pthread_create() return code: %d\n",t1);
				exit(EXIT_FAILURE);
			}	

	pthread_join(thread4,NULL);
	pthread_join(thread5,NULL);
}

void *readInput(){
	char *ch;
	int check=0;
	int cpid;
	char temp_buf[256];
	int n;
	write(STDOUT_FILENO, "Enter server commands:(list, print, printclient *client PID*)\n",62);

	while(1){
		
		n= read(STDIN_FILENO, buf2, 1024);
		if(n==1){
		write(STDOUT_FILENO,"Please enter a command\n",23);
		} 
		else{
		memcpy(temp_buf,buf2,256);
		temp_buf[n-1]='\0';
		ch=strtok(temp_buf," ");
		if(ch==NULL){
			write(STDOUT_FILENO,"Please enter a command\n",23);
		}
		else{
		if(strcmp(ch,"list")==0 || strcmp(ch,"print")==0 ||strcmp(ch,"printclient")==0){
			if(clistsize==0){
				write(STDOUT_FILENO,"No clients connected atm\n",25);
			}
			else{
				if(strcmp(ch,"list")==0 || strcmp(ch,"print")==0){	
					for(int i=0; i<clistsize; ++i)
						{
			  			write(clientlist[i].clientwrite,buf2,n);					
						}
					}
				else if(strcmp(ch,"printclient")==0){
					ch=strtok(NULL," ");
					if(ch==NULL){
						write(STDOUT_FILENO,"Please enter PID of client and the msg\n",39);
					}
					else{
						sscanf(ch,"%d", &cpid);

						for(int i=0; i<clistsize; ++i){
							if(clientlist[i].clientpid==cpid){
								check++;
								write(clientlist[i].clientwrite,buf2,n);
							}
						}
						if(check==0){
							write(STDOUT_FILENO,"Can not find client with the given PID\n",39);
						}	
					}
					
				}
			}	
		}
		
		else{
			write(STDOUT_FILENO,"Invalid command\n",16);
		}
		
	}
}
	}
}

void *writeList(void *p){
	int * a=(int*)p;
	int serverreadpipe=a[0];
	char buff[1024];
				
		while(1){
			int x=read(serverreadpipe,buff,1024);  //reading from CH pipe in conn
			if(buff==NULL){
				write(STDOUT_FILENO,"No processes created atm\n",25);
			}
			else{
				write(STDOUT_FILENO,"ParentPID\tchildPID\tName\t\tactiveStatus\tStart Time\tEnd Time\tElapsed Time\n",71);
				if(x!=0)
				{	
					write(STDOUT_FILENO,buff,x);
				}
			}
			
			
		}		
}

void * handleServerInput(void * p){
	int * arrServer= (int*)p;
	serverInputfunc(arrServer[0],arrServer[1],arrServer[2]);
}

void serverInputfunc(int clientreadpipe, int clientfd, int serverwritepipe){
	char inputread[1024];
	char *ch;
	char printmsg[1024];

	while(1){
		int n=read(clientreadpipe, inputread, 1024);      //reading from conn pipe in CH 
		inputread[n-1]='\0'; 
		ch=strtok(inputread, " ");
		
			if(strcmp(ch,"list")==0){
				if(listSize==0){
					write(STDOUT_FILENO,"No processes created atm by any client\n",39);
				}
				else{
			for(int i=0; i <listSize;++i){
				int ncount= sprintf(buf3, "%d\t\t%d\t\t%s\t\t%d\t\t%ld\t%ld\t%d\n",t1[i].parentPID,t1[i].childPID,t1[i].name, t1[i].activeStatus, t1[i].start_time,t1[i].end_time, t1[i].elapsed_time);
				write(serverwritepipe,buf3,ncount);      //writing to conn pipe from CH (CH --> conn)
			}
			}
		}
		else if(strcmp(ch,"print")==0 ){
			
			write(clientfd,"Msg from server:\n",17);
			ch=strtok(NULL," ");
			while (ch!= NULL)
                {	
                	int n=sprintf(printmsg,"%s",ch);
                	write(clientfd,printmsg,n);
                	write(clientfd," ",1);
                	ch=strtok(NULL," ");
                }
                write(clientfd,"\n",1);
		}
		else if(strcmp(ch,"printclient")==0){
			ch=strtok(NULL," ");
			write(clientfd,"Msg from server:\n",17);
			ch=strtok(NULL," ");
			while (ch!= NULL)
            {	
                int n=sprintf(printmsg,"%s",ch);
               	write(clientfd,printmsg,n);
               	write(clientfd," ",1);
               	ch=strtok(NULL," ");
            }
            write(clientfd,"\n",1);
		}
		
	}
}
void *takeClientInput(){
do {
 			if ((rval = read(client_sockfd, buf, 1024)) < 0)
				perror("reading stream message");
			if (rval == 0)
				write(STDOUT_FILENO,"Ending connection\n",18);
				
			else{
				write(STDOUT_FILENO,"-->",3);
				write(STDOUT_FILENO,buf, rval);
				write(STDOUT_FILENO,"\n",1);
				buf[rval-1]='\0';
				ch=strtok(buf," ");

				if (strcmp(ch,"run")==0){
					run();	
				}
				else if (strcmp(ch,"add")==0){
					add();
				}
				else if(strcmp(ch,"sub")==0){
					sub();
				}
				else if(strcmp(ch,"mult")==0){
					multiply();	
				}
				else if(strcmp(ch,"div")==0){
					divide();
				}
				else if(strcmp(ch,"list")==0){
					list();
				}
				else if(strcmp(ch,"kill")==0){
					Kill();
				}
				else if(strcmp(ch,"print")==0){
					print();
				}
				else if(strcmp(ch,"exit")==0){
					Exit();	
				}
				// else
				// {
				// 	write(STDOUT_FILENO,"Invalid command. Re-enter!\n",27);
				// }
			}

		}while (rval != 0);
		
}

void add(){
	float sum=0;
	float tok=0;
	char fsum[100];
	
		while((ch=strtok(NULL," "))!=NULL)
	  	{	
	  		sscanf(ch,"%f",&tok);
	  		sum+=tok;
	  	}
	  	count=sprintf(fsum,"%f",sum);
	  	write(client_sockfd,fsum, count);
		write(client_sockfd,"\n",1);  
}

void sub(){
	ch=strtok(NULL," ");
	if(ch==NULL){
			write(client_sockfd,"Please enter complete command\n",30);
	}
	else{

	
	float sum=0;
	float tok=0;
	char fsum[100];
	sscanf(ch,"%f",&sum);
	while((ch=strtok(NULL," "))!=NULL)
		{	
			sscanf(ch,"%f",&tok);
			sum-=tok;
		}
		count=sprintf(fsum,"%f",sum);
		write(client_sockfd,fsum, count);
		write(client_sockfd,"\n",1);
		}
}

void multiply(){
	ch=strtok(NULL," ");
	if(ch==NULL){
			write(client_sockfd,"Please enter complete command\n",30);
		}
	else{

		
	float sum=0;
	float tok=0;
	char fsum[100];
	sscanf(ch,"%f",&sum);
	
		while((ch=strtok(NULL," "))!=NULL){
			sscanf(ch,"%f",&tok);
			sum*=tok;
		}
		count=sprintf(fsum,"%f",sum);
		write(client_sockfd,fsum, count);
		write(client_sockfd,"\n",1);
}
}
void divide(){
	ch=strtok(NULL," ");
	if(ch==NULL){
			write(client_sockfd,"Please enter complete command\n",30);
	}
	else{
	float sum=0;
	float tok=0;
	char fsum[100];
	sscanf(ch,"%f",&sum);
	
		while((ch=strtok(NULL," "))!=NULL){
			sscanf(ch,"%f",&tok);
			if(tok==0){
				write(client_sockfd,"Can not divide by zero\n",23);
				return;
			}else{
				sum/=tok;
			}
			
		}
		count=sprintf(fsum,"%f",sum);
		write(client_sockfd,fsum, count);
		write(client_sockfd,"\n",1);
	}
}
void run(){

	int execReturn;
	int wcount;
	int pipe3[2];  //to read child pid in parent
	pipe(pipe3);
	int pipefd2[2];  //to read exec return value in parent

	int p=pipe2(pipefd2,__O_CLOEXEC); 
	if(p==-1){
		perror("pipe error");
	}
	char childpid[10];
	int pid2= fork();

	if(pid2==-1){
		perror("fork error");
	}

	if(pid2>0){   
		close(pipefd2[1]);                //CLIENT HANDLER
		write(pipefd[1],buf, rval);
		char execret[5];
		int rcount=read(pipefd2[0],execret,5);
		if(rcount==-1){
			perror("read failed");
		}
		else if(rcount==0){
			int parent=getpid();
			char parentarr[10];
			int par=sprintf(parentarr,"%d\n",parent);
			write(STDOUT_FILENO,"parent PID:",11);
			write(STDOUT_FILENO,parentarr,par);
			write(client_sockfd,"Process entry made in list\n",27);
			t1[listSize].parentPID=getpid();  
			read(pipe3[0],childpid,sizeof(childpid));
			t1[listSize].childPID=atoi(childpid);
			ch=strtok(NULL, " ");
			strcpy(t1[listSize].name, ch);
			t1[listSize].activeStatus=1;
			t1[listSize].start_time=time(NULL);
			listSize++;
			
		}
		else if(rcount>0){
				write(client_sockfd,"Invalid process name entered\n",29);
		}
				
				
	} else if(pid2==0){     						//child

				close(pipefd2[0]);
				count=read(pipefd[0],buf,sizeof(buf)); //reading from msgsock                  
				ch=strtok(NULL, " ");
				if(ch==NULL){
				write(client_sockfd,"Please enter application name\n",30);

				}
				int child=getpid();
				count= sprintf(childpid,"%d",child);
				write(pipe3[1],childpid,count);
				while(ch!=NULL)
				{ 
					char *args[]={ch,NULL};
					execReturn =execvp(args[0],args);
					if(execReturn==-1){
						wcount=write(pipefd2[1],"abc\n",4);

				}
				}
			}	
	}

void list(){
	if(listSize==0){
		write(client_sockfd,"No processes created atm\n",25);
	}
	else{
	char buf[1024];
  	//write(STDOUT_FILENO,"childPID\tparentPID\tName\t\tactiveStatus\tStart Time\tEnd Time\tElapsed Time\n",71);
  	write(client_sockfd,"childPID\tparentPID\tName\t\tactiveStatus\tStart Time\tEnd Time\tElapsed Time\n",71);
  	for(int i=0; i<listSize; ++i){
  		count= sprintf(buf, "%d\t\t%d\t\t%s\t\t%d\t\t%ld\t%ld\t%d\n",t1[i].childPID, t1[i].parentPID,t1[i].name, t1[i].activeStatus, t1[i].start_time,t1[i].end_time, t1[i].elapsed_time);
		write(client_sockfd,buf,count);
		write(client_sockfd,"\n",1);
		
  }	
	}
	
}
 
void print(){
	char printmsg[100];
   	write(STDOUT_FILENO,"msg from client: ",17);
   	ch=strtok(NULL," ");
		while (ch!= NULL)
        {	
            int n=sprintf(printmsg,"%s",ch);
            write(STDOUT_FILENO,printmsg,n);
            write(STDOUT_FILENO," ",1);
            ch=strtok(NULL," ");
        }
  	write(STDOUT_FILENO,"\n",1);	 
   	write(client_sockfd,"print msg sent to server\n",25);
  
  
}
void Kill(){
	ch=strtok(NULL, " ");
	if(ch==NULL){
		write(client_sockfd,"Please enter PID of process to kill\n",36);
	}
	else{
		int counterc=0;
		int conv=atoi(ch);  
		for (int i = 0; i < listSize; ++i)
			{   if (conv==t1[i].childPID || strcmp(ch,t1[i].name)==0)
				{    
					counterc++;
					if (t1[i].activeStatus==1)
					{
						kill(t1[i].childPID, SIGKILL);
						t1[i].activeStatus=0;
						t1[i].end_time=time(NULL);
						t1[i].elapsed_time= t1[i].end_time - t1[i].start_time;
						write(STDOUT_FILENO,"Kill successful! Updated in list.\n",35);
						write(client_sockfd,"Kill successful! Updated in list.\n",35);
						break;
					}
					else
					{
						write(client_sockfd,"Process already killed\n",23);
					}
				}
			} 
			if(counterc==0){
				write(client_sockfd,"PID not found in list. Re-enter!\n",33);
			} 
	}
		
}

void Exit(){
	write(client_sockfd,"Closing all processes...\n",25);
	kill(getpid(), SIGKILL);
}