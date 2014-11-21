#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

#define CMD_MAX 1000
void broadcast(int clients[], int maxi, const char *msg)
{
	int i;
	char buf[CMD_MAX];
	sprintf(buf, "[Server] %s\n", msg);
	for(i = 0; i <= maxi; i++)
		if(clients[i] >= 0)
			write(clients[i], buf, strlen(buf));
}

int main(int argc,char *argv[])
{
    if(argc!=2)
	{
		fprintf(stderr, "Usage: %s <port>\n",argv[0]);
		exit(1);		
	}
	int listen_fd;
	int clients[FD_SETSIZE];
	int client_maxi = 0;
	int max_fd;
	char name[FD_SETSIZE][20];
	char ip[FD_SETSIZE][20];
	short port[FD_SETSIZE];
	
	memset(clients , 0xFF, sizeof(clients));
	
	struct sockaddr_in listen_addr;
	memset(&listen_addr, 0, sizeof(listen_addr));
	listen_addr.sin_family = AF_INET;
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_port = htons(atoi(argv[1]));
		
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd == -1)
	{
		fputs("Cannot create socket.\n",stderr);
		exit(2);
	}
	if(bind(listen_fd, (struct sockaddr *) &listen_addr, sizeof(listen_addr)) == -1)
	{
		fputs("Cannot bind.\n",stderr);
		exit(2);
	}
	if(listen(listen_fd ,1024)== -1)
	{
		fputs("Cannot listen.\n",stderr);
		exit(2);
	}
	
	fd_set all_set, r_set;
	FD_ZERO(&all_set);
	FD_ZERO(&r_set);
	FD_SET(listen_fd, &all_set);
	max_fd = listen_fd + 1;
	
	while(1)
	{
		r_set = all_set;
		int nready = select(max_fd, &r_set, NULL, NULL, NULL);
		
		if(FD_ISSET(listen_fd, &r_set))
		{
			struct sockaddr_in client_addr;
			int client_len = sizeof(client_addr);
			int client_fd = accept(listen_fd, (struct sockaddr *) &client_addr , &client_len);
			int i;
			
			broadcast(clients, client_maxi, "Someone is coming!");
			for(i=0;i<FD_SETSIZE;i++)
			{
				if(clients[i]<0)
				{
					clients[i]=client_fd;
					break;
				}
			}
			char welcome[CMD_MAX];
			strcpy(name[i], "anonymous");
			inet_ntop(AF_INET, &client_addr.sin_addr, ip[i], sizeof(ip[i]));
			port[i] = ntohs(client_addr.sin_port);
			sprintf(welcome, "[Server] Hello, anonymous! From: %s/%hu\n", ip[i], port[i]);
			write(clients[i], welcome, strlen(welcome));
			
			if(i == FD_SETSIZE)
			{
				printf("Too many!\n");
				exit(2);
			}
			if(i>client_maxi)
				client_maxi=i;
			if(client_fd>=max_fd)
				max_fd = client_fd + 1;
			
			FD_SET(client_fd, &all_set);
			if(!--nready)
				continue;
		}
		int i;
		for(i=0;i<=client_maxi&&nready;i++)
		{
			if(FD_ISSET(clients[i], &r_set))
			{
				--nready;
				
				char buff[CMD_MAX];	
				char send[CMD_MAX +1];
				int n = read(clients[i],buff,999);
				
				if(n == 0)
				{
					close(clients[i]);
					FD_CLR(clients[i], &all_set);
					clients[i] = -1;
					sprintf(send, "%s is offline.", name[i]);
					broadcast(clients, client_maxi, send);
				
				}
				else
				{
					buff[n] = '\0';		
					char err[] = "[Server] ERROR: Error command.\n";
					char *str = strtok(buff, " \n");
					
					if(str == NULL)
					;
					else if(strcmp("who",str) == 0)
					{
						int j;
						for(j=0;j<=client_maxi;j++)
						{	
							if(clients[j] < 0)
								continue;
							sprintf(send, "[Server] %s %s/%hu", name[j], ip[j], port[j]);
							if(j == i)
								strcat(send, " <-me\n");
							else
								strcat(send, "\n");
							write(clients[i], send, strlen(send));
						}
					}
					else if(strcmp("name",str) == 0)
					{
						str = strtok(NULL,"\n");
						char test[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
						if(strlen(str)==strspn(str,test) && strlen(str)>=2 && strlen(str)<=12)
						{
							if(strcmp(str,"anonymous")==0)
								sprintf(send, "[Server] ERROR: Username cannot be anonymous.\n" );
							else
							{
								int j, used = 0;
								char other[CMD_MAX];
								for(j=0;j<=client_maxi;j++)
									if(strcmp(str,name[j])==0 && j!=i)
									{
										sprintf(send, "[Server] ERROR: %s has been used by others.\n", str);									
										used = 1;
										break;
									}
								if(!used)
								{
									int save = clients[i];
									sprintf(other, "%s is now known as %s.",name[i], str);
									clients[i] = -1;
									broadcast(clients, client_maxi, other);
									clients[i] = save;
									strcpy(name[i],str);
									sprintf(send, "[Server] You're now known as %s.\n",str);
								}							
							}					
						}
						else
							sprintf(send, "[Server] ERROR: Username can only consists of 2~12 English letters.\n" );
						write(clients[i], send, strlen(send));
					}
					else if(strcmp("yell",str) == 0)
					{
						str = strtok(NULL,"\n");
						if(!str)
						{
							write(clients[i], err, strlen(err));
							break;
						}
						sprintf(send, "%s yell %s", name[i], str);
						broadcast(clients, client_maxi, send);						
					}
					else if(strcmp("tell",str) == 0)
					{
						int tar = -1, j;
						char target[CMD_MAX];
						char success[] = "[Server] SUCCESS: Your message has been sent.\n";
						char fail[] = "[Server] ERROR: The receiver doesn't exist.\n";
						char send_ano[] = "[Server] ERROR: You are anonymous.\n";
						char rec_ano[] = "[Server] ERROR: The client to which you sent is anonymous.\n";
						
						str = strtok(NULL," ");
						strcpy(target, str);
						for(j=0;j<=client_maxi;j++)
							if(strcmp(target,name[j])==0 && j!=i)
							{
								tar = j;
								break;
							}		
						if(strcmp(target,"anonymous")==0)
							strcpy(send,rec_ano);
						else if(strcmp(name[i],"anonymous")==0)
							strcpy(send,send_ano);
						else if(tar == -1)
							strcpy(send,fail);
						else
						{
							str = strtok(NULL,"\n");
							sprintf(send, "[Server] %s tell you %s\n", name[i], str);
							write(clients[tar], send, strlen(send));
							strcpy(send,success);
						}					
						write(clients[i], send, strlen(send));
					}
					else
						write(clients[i], err, strlen(err));					
								
				}	
			}
		}		
	}
}

