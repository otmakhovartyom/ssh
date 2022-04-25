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

int broadcast_client(in_addr_t client_addr, in_port_t broadcast_port);

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		printf("Wrong format. Correct input:\n./broadcast_client <client host> <broadcast port>\n");
		exit(EXIT_SUCCESS);
	}
	
	struct sockaddr_in serv_addr = {};
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));

	if (inet_ntop(AF_INET, &serv_addr.sin_addr, argv[1], strlen(argv[1])) == NULL)
	{
		perror("Wrong format. Correct input:\n./broadcast_client <client host> <broadcast port>\n");
		exit(EXIT_FAILURE);
	}

	broadcast_client(serv_addr.sin_addr.s_addr, serv_addr.sin_port);
	return 0;
}

int broadcast_client(in_addr_t client_addr, in_port_t broadcast_port)
{
	int client_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (client_sockfd == -1)
	{
		perror("socket error");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = client_addr, .sin_port = broadcast_port};

	if (bind(client_sockfd, (struct sockaddr*) &client, sizeof(client)) == -1)
	{
		perror("bind error");
		exit(EXIT_FAILURE);
	}
	
	struct sockaddr_in server = {};
	socklen_t server_len = sizeof(server);

	char buffer[BUF_LEN];
	int recsz = recvfrom(client_sockfd, buffer, BUF_LEN, 0, (struct sockaddr*)&server, &server_len);

	if (recsz == -1)
	{
		perror("recvfrom error");
		exit(EXIT_FAILURE);
	}

	buffer[recsz] = '\0';
	if (buffer[recsz - 1] == '\n')
		buffer[recsz - 1] = '\0';
	if (buffer[recsz - 2] == '\n')
		buffer[recsz - 2] = '\0';

	printf("Broadcast from %s:%d\n%s\n", inet_ntoa(((struct sockaddr_in*)&server)->sin_addr), ntohs(((struct sockaddr_in*)&server)->sin_port), buffer);

	printf("Enter a response to the server:\n");

	int len = read(STDIN_FILENO, buffer, BUF_LEN);
	
	if (buffer[len - 1] != '\n')
		printf("\n");

	if (len == -1)
	{
		perror("read error");
		exit(EXIT_FAILURE);
	}

	buffer[len] = '\0';
	
	int sending = sendto(client_sockfd, buffer, len + 1, 0, (struct sockaddr*)&server, server_len);

	if (sending == -1)
	{
		perror("sendto error");
		exit(EXIT_FAILURE);
	}

	close(client_sockfd);
	return 0;
}