#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>

#define CMD_MAX 1000

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: %s <ip> <port>\n", argv[0]);
		exit(1);
	}

	struct sockaddr_in server_addr;
	int sock_fd;
	int stdin_fd = fileno(stdin);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
	server_addr.sin_port = htons(atoi(argv[2]));

	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd == -1) {
		fputs("Fail to create socket.\n", stderr);
		exit(2);
	}

	if(connect(sock_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1) {
		fputs("Fail to connect.\n", stderr);
		exit(2);
	}

	char buf[CMD_MAX];
	fd_set r_set;
	FD_ZERO(&r_set);
	int max_fd;

		
	while(1) {
		FD_SET(stdin_fd, &r_set);
		FD_SET(sock_fd, &r_set);
		
		if(sock_fd > stdin_fd)
			max_fd = sock_fd + 1;
		else
			max_fd = stdin_fd + 1;
			
		select(max_fd, &r_set, NULL, NULL, NULL);

		if(FD_ISSET(stdin_fd, &r_set)) {
			int n = read(stdin_fd, buf, CMD_MAX);
			buf[n] = '\0';
			if(strcmp(buf, "exit\n") == 0) {
				close(sock_fd);
				break;
			}
			write(sock_fd, buf, n);
		}
		if(FD_ISSET(sock_fd, &r_set)) {
			int n = read(sock_fd, buf, CMD_MAX);
			if(n == 0) {
				puts("Connection closed by server");
				close(sock_fd);
				break;
			}
			buf[n] = '\0';
			fputs(buf, stdout);
		}
	}

	return 0;
}
