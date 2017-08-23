/* Useful function signatures
int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
int socket(int domain, int type, int protocol);
int getaddrinfo(const char *node, const char *service,const struct addrinfo *hints,struct addrinfo **res);
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);
int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout); */

#include <stdio.h>
#include <stdlib.h>		// exit()
#include <sys/socket.h>	// connect() socket()
#include <unistd.h>		// close() read() write()
#include <netdb.h>		// getaddrinfo()
#include <sys/select.h> // select()

#define WILL 251
#define WONT 252
#define DO   253
#define DONT 254
#define IAC  255

int getConnected(char* host,char* port)
{
	struct addrinfo hint; struct addrinfo *res;
	int skt; // socket file discripter

    hint.ai_family=AF_UNSPEC;	 	// allow ipv4 and ipv6 
    hint.ai_socktype=SOCK_STREAM; 	// for tcp connection 
    hint.ai_protocol=0;				// any protocol
    hint.ai_flags=0;				// no flags set
	
	int te=getaddrinfo(host,port,&hint,&res);
    if(te!=0){ printf("could not resolve %s: %s\n",host,gai_strerror(te)); }

    struct addrinfo *i;
    for(i=res;i!=NULL;i=i->ai_next)
    {
    	skt=socket(i->ai_family,i->ai_socktype,i->ai_protocol);
    	if(skt==-1) continue; // socket connection failed
    	
    	char ip[40]; 
    	if(getnameinfo(i->ai_addr,i->ai_addrlen,ip, sizeof(ip)-1,NULL,0,NI_NUMERICHOST)==0)
    		printf("Trying %s\n",ip);
    	
    	if(connect(skt,i->ai_addr,i->ai_addrlen)!=-1) break; // found
    	close(skt);
    }

    if(i==NULL){ printf("Connection Failure\n"); exit(1); }
    freeaddrinfo(res);

    printf("Connected to %s\n",host);
    return skt;
}

void sendComm(int skt,uint8_t optCode,uint8_t code)
{
	uint8_t comm[3]; comm[0]=IAC; comm[1]=optCode; comm[2]=code;
	if(write(skt,comm,3)<0) printf("write error\n"),exit(1);
}

void readComm(int skt)
{
	uint8_t buff[2];
	if(read(skt,buff,2)<2) printf("read error\n"),exit(1);
	//printf("%d %d\n",buff[0],buff[1]);
	
	if(buff[0]==WILL) sendComm(skt,DONT,buff[1]);
	if(buff[0]==DO)   sendComm(skt,WONT,buff[1]);
}

int main(int argc,char* argv[])
{
	if(argc<2||argc>3) { printf("Usage: telnet HOSTNAME PORT(optional)\n"); return 0; }
	
	struct timeval wait; wait.tv_usec=500; wait.tv_sec=0; uint8_t buff[200];
	int skt=getConnected(argv[1],(argc==3?argv[2]:(char*)"23"));
	
	sendComm(skt,DONT,1); 	// suppress server echo (telehack.com DOES NOT honor this request)

	while(1)
	{
		fd_set readSet; FD_ZERO(&readSet);
		FD_SET(skt,&readSet); FD_SET(STDIN_FILENO,&readSet);

		int sel=select(skt+1,&readSet,NULL,NULL,&wait);

		if(sel==-1) printf("Select error\n"),exit(1);
		else if(FD_ISSET(skt,&readSet)) // socket ready to read
		{
			if(read(skt,buff,1)<1) break;
			else if(buff[0]==IAC) readComm(skt);
			else printf("%c",buff[0]);
			fflush(0);
		}
		else if(FD_ISSET(STDIN_FILENO,&readSet)) // stdin ready to read
		{
			int c=0; scanf("%c",&buff[c]);
			while(buff[c]!='\n') scanf("%c",&buff[++c]);
			buff[++c]='\r'; // to complete escape character

			if(write(skt,buff,c+1)<0) printf("write error\n"),exit(1);
		}
	}

	printf("Connection closed\n"); close(skt);
	return 0;
}