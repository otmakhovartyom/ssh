#include "ssh_lib.h"

int broadcast_waiting(in_addr_t serv_addr, in_port_t serv_port)
{
	int server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (server_socket == -1)
	{
		perror("socket error");
		return ERROR_SOCKET;
	}

	int reuse = 1;
	int broadcast = 1;
	struct timeval time = {.tv_sec = 120};

	if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int)) == -1) ||
		(setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
		(setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time)) == -1))
	{
		perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = serv_addr, .sin_port = serv_port};

	if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
	{
		perror("bind error");
		return ERROR_BIND;
	}

	struct sockaddr_in client = {};
	socklen_t client_len = sizeof(server);

	char buffer[BUF_LEN] = "";
	
	int recsize = recvfrom(server_socket, buffer, BUF_LEN - 1, 0, (struct sockaddr*)&client, &client_len);
	if (recsize == -1)
	{
		perror("recvfrom error");
		return ERROR_RECVFROM;
	}

	buffer[recsize] = '\0';

	if (strcmp(buffer, "Where is server?") == 0)
	{
		static const char msg[] = "I'm here!";

		int data = sendto(server_socket, msg, sizeof(msg), 0, (struct sockaddr*)&client, client_len);
		
		if (data == -1)
		{
			perror("send error");
			return  ERROR_SEND;
		}
	}

	close(server_socket);
	return 0;
}

int main()
{
	struct sockaddr_in client;
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(27312);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
}