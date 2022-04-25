#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define BUF_LEN 1000

int client_tcp_protocol(in_addr_t serv_addr, in_port_t serv_port);
int client_udp_protocol(in_addr_t serv_addr, in_port_t serv_port);

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		printf("Wrong format. Correct input: ./client [-tcp | -udp] <host> <port>\n");
		exit(EXIT_SUCCESS);
	}
	
	if (strcmp(argv[1], "-tcp") != 0 && strcmp(argv[1], "-udp") != 0)
	{
		printf("Wrong format. Correct input: ./client [-tcp | -udp] <host> <port>\n");
		exit(EXIT_SUCCESS);
	}
	
	struct sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	
	if (inet_pton(AF_INET, argv[2], &serv_addr.sin_addr) != 1)
	{
		perror("Wrong format. Correct input: ./client [-tcp | -udp] <host> <port>\n");
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[1], "-tcp") == 0)
		client_tcp_protocol(serv_addr.sin_addr.s_addr, serv_addr.sin_port);

	if (strcmp(argv[1], "-udp") == 0)
		client_udp_protocol(serv_addr.sin_addr.s_addr, serv_addr.sin_port);

	return 0;
}

int client_tcp_protocol(in_addr_t serv_addr, in_port_t serv_port)
{
	int client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if (client_sockfd == -1)
	{
		perror("socket error");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = serv_addr;
	addr.sin_port = serv_port;
	
	if (connect(client_sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1)
	{
		perror("connect error");
		exit(EXIT_FAILURE);
	}

    printf("Enter the message:\n");
	
	char buffer[BUF_LEN];
	int len = read(STDIN_FILENO, buffer, BUF_LEN);
	
	if (len == -1)
	{
		perror("read error");
		exit(EXIT_FAILURE);
	}
	
	buffer[len] = '\0';
	
	int sending = send(client_sockfd, buffer, len + 1, 0);
	
	if (sending == -1)
	{
		perror("send error");
		exit(EXIT_FAILURE);
    }
	
	close(client_sockfd);
	return 0;
}

int client_udp_protocol(in_addr_t serv_addr, in_port_t serv_port)
{
	int client_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (client_sockfd == -1)
	{
		perror("socket error");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = serv_addr;
	addr.sin_port = serv_port;

	printf("Enter the message:\n");
	
	char buffer[BUF_LEN];
	int len = read(STDIN_FILENO, buffer, BUF_LEN);
	
	if (len == -1)
	{
		perror("read error");
		exit(EXIT_FAILURE);
	}
	
	buffer[len] = '\0';
	
	int sending = sendto(client_sockfd, buffer, len + 1, 0, (struct sockaddr*)&addr, sizeof(addr));
	
	if (sending == -1)
	{
		perror("send error");
		exit(EXIT_FAILURE);
	}
	
	close(client_sockfd);
	return 0;
}