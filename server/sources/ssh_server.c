#include "../headers/ssh_lib.h"
#include "../headers/server.h"

int main(int argc, char *argv[])
{
//	int haha = fork();

	// if (haha)
	// {
	// 	while (1)
	// 	{
	// 		// I AM HAPPY !!!
	// 	}
		
	// }
	
	// else
	// {
	// 	setsid();
	// 	printf("DAEMON:%d\n", getpid());
	// 	kill(getppid(), SIGKILL);
	// }
	
	if (argc != 3)
	{
		printf("Please, run the program in the following format: \n"               "./server <host> <port>\n");
		return ERROR_INVALID_ARGC;
	}

	// int f = fork();

	// if (!f)
	// {
		struct sockaddr_in address = {};

		if (inet_ntop(AF_INET, &address.sin_addr, argv[1], strlen(argv[1])) == NULL)
		{
			printf("Please, run the program in the following format: \n"               "./server <host> <port>\n");
			return ERROR_INVALID_ADDRESS;
		}

		address.sin_port = htons(atoi(argv[2]));

		int result = broadcast_waiting(address.sin_addr.s_addr, address.sin_port);

		if (result < 0)
		{
			log_perror("broadcast beda");
			exit(EXIT_FAILURE);
		}

		return 0;
	// }

// 	else
// 	{
// 		struct sockaddr_in server;
// 		memset(&server, '\0', sizeof(server));
// 		server.sin_family = AF_INET;
// 		server.sin_port = htons(SCP_PORT);
// 		struct sockaddr_in client = {};
// 		socklen_t client_len = sizeof(client);

// 		if (inet_pton(AF_INET, "192.168.0.106", &server.sin_addr) != 1)
// 		{
// 			log_perror("inet_pton error");
// 			exit(EXIT_FAILURE);
// 		}

// //		int server_socket = udp_server_socket_create(server.sin_addr.s_addr, server.sin_port);

// 		int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
	
// 		if (server_socket == -1)
// 		{
// 			log_perror("socket error");
// 			return ERROR_SOCKET;
// 		}

// 		if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
// 		{
// 			log_perror("bind error");
// 			return ERROR_BIND;
// 		}

// 		char buffer[BUF_LEN] = "";

// 		int recsize = recvfrom(server_socket, buffer, BUF_LEN - 1, 0, (struct sockaddr*)&client, &client_len);
// 		if (recsize == -1)
// 		{
// 			log_perror("recvfrom error");
// //			return ERROR_RECVFROM;
// 		}

// 		printf("buffer=%s\n");

// 		recsize = 0;
		
// 		while (1)
// 		{
// 		//	printf("hello_recv\n");
// 			recsize = recvfrom(server_socket, buffer, BUF_LEN - 1, MSG_WAITALL, (struct sockaddr*)&client, &client_len);
// 			perror("SASAT");
			
// 			if (recsize == -1)
// 			{
// 				log_perror("recvfrom error");
// //				return ERROR_RECVFROM;
// 			}
			
// 			if (recsize > 0)
// 			{
// 				printf("success_recv\n");
// 				break;
// 			}
// 		}

// 		buffer[recsize] = '\0';
		
// 		printf("hellostrcpmp\n");

// 		if (strcmp(buffer, "sending") == 0)
// 		{
// 			printf("success sending\n");

// 			if (get_file(server_socket, 1, &server) < 0)
// 			{
// 				printf("send_file error");
// 				exit(EXIT_FAILURE);
// 			}
// 		}

// 		return 0;
// 	}

}

int udp_server_socket_create(in_addr_t server_addr, in_port_t server_port)
{
	int server_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	
	if (server_socket == -1)
	{
		log_perror("socket error");
		return ERROR_SOCKET;
	}

	int reuse = 1;
	struct timeval time = {.tv_sec = 15};

	if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) ||
		(setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO,  &time, sizeof(time)) == -1))
	{
		log_perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = server_addr, .sin_port = server_port};

	if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
	{
		log_perror("bind error");
		return ERROR_BIND;
	}

	return server_socket;
}

int broadcast_waiting(in_addr_t serv_addr, in_port_t serv_port)
{
	int server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	if (server_socket == -1)
	{
		log_perror("socket error");
		return ERROR_SOCKET;
	}

	int reuse = 1;
	int broadcast = 1;
	struct timeval time = {.tv_sec = 120};

	if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,     sizeof(int)) == -1) ||
		(setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
		(setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO,  &time,      sizeof(time)) == -1))
	{
		log_perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = serv_addr, .sin_port = htons(BROADCAST_PORT)};
//	struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = serv_addr, .sin_port = htons(BROADCAST_PORT)};

	if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
	{
		log_perror("bind error");
		return ERROR_BIND;
	}

	struct sockaddr_in client = {};
	socklen_t client_len = sizeof(client);

	char buffer[BUF_LEN] = "";
	
	int recsize = recvfrom(server_socket, buffer, BUF_LEN - 1, 0, (struct sockaddr*)&client, &client_len);
	if (recsize == -1)
	{
		log_perror("recvfrom error");
		return ERROR_RECVFROM;
	}

	buffer[recsize] = '\0';

	if (strcmp(buffer, "Where is server?") == 0)
	{
		static const char msg[] = "I'm here!";

		int data = sendto(server_socket, msg, sizeof(msg), 0, (struct sockaddr*)&client, client_len);
		
		if (data == -1)
		{
			log_perror("send error");
			return  ERROR_SEND;
		}
	}

	close(server_socket);
	return 0;
}