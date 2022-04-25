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

int broadcast_server(in_addr_t serv_addr, in_port_t serv_port, in_addr_t broadcast_address, in_port_t broadcast_port);

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		printf("Wrong format. Correct input:\n./broadcast_server <server host> <server port> <broadcast port>\n");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	
	if (inet_ntop(AF_INET, &serv_addr.sin_addr, argv[1], strlen(argv[1])) == NULL)
	{
		printf("Wrong format. Correct input:\n./broadcast_server <server host> <server port> <broadcast port>\n");
		exit(EXIT_FAILURE);
	}
	
	broadcast_server(serv_addr.sin_addr.s_addr, serv_addr.sin_port, INADDR_BROADCAST, htons(atoi(argv[3])));	
	return 0;
}

int broadcast_server(in_addr_t serv_addr, in_port_t serv_port, in_addr_t broadcast_address, in_port_t broadcast_port)
{
	int server_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (server_sockfd == -1)
	{
		perror("socket error");
		exit(EXIT_FAILURE);
	}

	int reuse = 1;
	int broadcast = 1;
	struct timeval time = {.tv_sec = 120};

	if ((setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int)) == -1) ||
		(setsockopt(server_sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
		(setsockopt(server_sockfd, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time)) == -1))
	{
		perror("setsockopt error");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = broadcast_address, .sin_port = broadcast_port};
	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = serv_addr, .sin_port = serv_port};
	socklen_t client_len = sizeof(client);

	if (bind(server_sockfd, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
	{
		perror("bind error");
		exit(EXIT_FAILURE);
	}
	
	printf("Enter the message:\n");

	char buffer[BUF_LEN];
	int len = read(STDIN_FILENO, buffer, BUF_LEN);
	
	if (buffer[len - 1] != '\n')
		printf("\n");

	if (len == -1)
	{
		perror("read error");
		exit(EXIT_FAILURE);
	}

	buffer[len] = '\0';

	int sending = sendto(server_sockfd, buffer, len + 1, 0, (struct sockaddr*)&client, sizeof(client));

	if (sending == -1)
	{
		perror("send error");
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		int read_size = recvfrom(server_sockfd, buffer, BUF_LEN - 1, 0, (struct sockaddr*)&client, &client_len);

		if (read_size == -1)
		{
			perror("recvfrom error");
			close(server_sockfd);
			exit(EXIT_FAILURE);
		}

		buffer[read_size] = '\0';
		if (buffer[read_size - 1] == '\n')
			buffer[read_size - 1] = '\0';
		if (buffer[read_size - 2] == '\n')
			buffer[read_size - 2] = '\0';

		printf("Reply from %s:%u\n%s\n", inet_ntoa(((struct sockaddr_in*)&client)->sin_addr), ntohs(((struct sockaddr_in*)&client)->sin_port), buffer);
	}
}