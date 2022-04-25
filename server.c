#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>

#define BUF_LEN 10000

void antizombie(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

int server_socket = 0;

int server_tcp_protocol(in_addr_t serv_addr, in_port_t serv_port, int queue_sz);
int server_udp_protocol(in_addr_t serv_addr, in_port_t serv_port);

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		printf("Wrong format. Correct input: ./server [-tcp | -udp] <host> <port>\n");
		exit(EXIT_FAILURE);
	}
	
	if (strcmp(argv[1], "-tcp") != 0 && strcmp(argv[1], "-udp") != 0)
	{
		printf("Wrong format. Correct input: ./server [-tcp | -udp] <host> <port>\n");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	
	if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) != 1)
	{
		printf("Wrong format. Correct input: ./server [-tcp | -udp] <host> <port>\n");
		exit(EXIT_FAILURE);
	}
	
	if (strcmp(argv[1], "-tcp") == 0)
		server_tcp_protocol(serv_addr.sin_addr.s_addr, serv_addr.sin_port, 10);
	
	if (strcmp(argv[1], "-udp") == 0)
		server_udp_protocol(serv_addr.sin_addr.s_addr, serv_addr.sin_port);
	
	return 0;
}

int server_tcp_protocol(in_addr_t serv_addr, in_port_t serv_port, int queue_sz)
{
	if (queue_sz < 0)
		return 0;
	
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	
	if (server_socket == -1)
	{
		perror("socket error");
		exit(EXIT_FAILURE);
	}
	
	int perm = 1;
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &perm, sizeof(int)) == -1)
	{
		perror("setsockopt error");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = serv_addr, .sin_port = serv_port};
	
	if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
	{
		perror("bind error");
		exit(EXIT_FAILURE);
	}
	
	if (listen(server_socket, queue_sz) == -1)
	{
		perror("listen error");
		exit(EXIT_FAILURE);
	}
	
	struct sigaction act = {.sa_handler = antizombie, .sa_flags = SA_RESTART};
	sigemptyset(&act.sa_mask);
	
	if (sigaction(SIGCHLD, &act, NULL) == -1)
	{
		perror("sigaction error");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_storage client_address = {};
	socklen_t client_address_len = sizeof(client_address);
	
	while (1)
	{
		int client_fd = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
		if (client_fd == -1)
		{
			perror("accept error");
			continue;
		}
		
		if (!fork())
		{
			close(server_socket);
			
			char buffer[BUF_LEN];
			int read_size = 0;
			
			while ((read_size = recv(client_fd, buffer, BUF_LEN - 1, 0)) != -1)
			{
				if (read_size == 0) break;
				
				buffer[read_size] = '\0';
				printf("%s:%u\n%s\n", inet_ntoa(((struct sockaddr_in*)&client_address)->sin_addr), ntohs(((struct sockaddr_in*)&client_address)->sin_port), buffer);
				memset(buffer, 0, BUF_LEN);
			}
			
			close(client_fd);
			return 0;
		}
		close(client_fd);
	}

	return 0;
}

int server_udp_protocol(in_addr_t serv_addr, in_port_t serv_port)
{
	server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (server_socket == -1)
	{
		perror("socket error");
		exit(EXIT_FAILURE);
	}
	
	int perm = 1;
	
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &perm, sizeof(int)) == -1)
	{
		perror("setsockopt error");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = serv_addr, .sin_port = serv_port};

	if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
	{
		perror("bind error");
		exit(EXIT_FAILURE);
	}

	char buffer[BUF_LEN];
	struct sockaddr client_address = {};
	socklen_t client_address_len = sizeof(client_address);

	while (1)
	{
		int read_size = recvfrom(server_socket, buffer, BUF_LEN - 1, 0, &client_address, &client_address_len);

		if (read_size == -1)
		{
			perror("recvfrom error");
			exit(EXIT_FAILURE);
		}

		buffer[read_size] = '\0';

		printf("%s:%u\n", inet_ntoa(((struct sockaddr_in*)&client_address)->sin_addr), ntohs(((struct sockaddr_in*)&client_address)->sin_port));
		
		fflush(stdout);
		if (write(STDOUT_FILENO, buffer, read_size) == -1)
		{
			perror("write error");
			exit(EXIT_FAILURE);
		}

		printf("\n");
		memset(buffer, 0, BUF_LEN);
	}
}
