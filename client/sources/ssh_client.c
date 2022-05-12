#include "../headers/ssh_lib.h"
#include "../headers/client.h"

int main(int argc, char *argv[])
{
	if (argc < 2)
    {
        help_message();
        return 0;
    }

	size_t i = 1;
	in_addr_t serv_addr = 0;
	char username[100] = "";

	if (strcmp(argv[i], "--broadcast") == 0 || (strcmp(argv[i], "-b")) == 0)
	{
		if (broadcast_search(INADDR_ANY, htons(BROADCAST_PORT)) < 0)
		{
			perror("broadcast_search error");
			exit(EXIT_FAILURE);
		}

		return 0;
	}

	if (strcmp(argv[i], "-scp") == 0)
	{
		printf("helloscp\n");
		struct sockaddr_in server = {.sin_family = AF_INET, .sin_port = htons(SCP_PORT)};
//		socklen_t server_len = sizeof(server);

		if (inet_pton(AF_INET, argv[i + 2], &server.sin_addr) != 1)
		{
			printf("inet_pton error\n");
			exit(EXIT_FAILURE);
		}

		int client_socket = udp_client_socket_create(server.sin_addr.s_addr, server.sin_port);

		if (client_socket < 0)
		{
			perror("file_socket_create error");
			exit(EXIT_FAILURE);
		}

		if (sendto(client_socket, "sending", strlen("sending"), 0, (struct sockaddr*)&server, sizeof(server)) < 0)
		{
			log_perror("sendfile error");
			return ERROR_SEND;
		}

		if (send_file(argv[i + 1], argv[i + 3], client_socket, 1, &server) < 0)
		{
			printf("send_file error");
			exit(EXIT_FAILURE);
		}

		return 0;
	}

	if (strcmp(argv[i], "-ssh") == 0)
	{
		++i;

		if (i < argc)
		{
			char *addr = strchr(argv[i], '@');

			if (addr == NULL)
			{
				help_message();
				return 0;
			}

			++addr;

			serv_addr = inet_addr(addr);

			if (serv_addr == INADDR_NONE)
			{
				help_message();
				return 0;
			}

			strncpy(username, argv[i], addr - 1	- argv[i]);
			++i;
		}

		else
		{
			help_message();
			return 0;
		}

		if (client_operation(SOCK_STREAM, serv_addr, username) < 0)
		{
			log_perror("client_operation error");
			exit(EXIT_FAILURE);
		}
	}

	return 0;
}

void alarm_handler(int signum)
{
    alarm_wait = 1;
}

void help_message()
{
	printf("Usage: ./ssh_client [command] [options]...\n"
		"Commands:\n"
		"[ -b | --broadcast ] [ <port> ]                       Find accessible SSH servers using broadcast by <port> and write <ip:port> to \"/tmp/.ssh_broadcast\".\n"
		"                                                      Default broadcast <port> is 35000.\n"
		"[ -h | --help ]                                       Dispay this information.\n"
		"[ -scp ] [ client path ] [ <host> ] [ server path ]   Copy your file to server.\n"
		"\n"
		"Options:\n"
		"[ -j | --systemlog | -f <file> | --filelog <file> ]   Logging to the system journal or to <file>.\n"
		"                                                      Default mode is system journaling.\n"
		"                                                      Default log <file> is \"/tmp/.ssh-log\".\n");
}

int timer_creater()
{
	sigset_t sig = {};
	sigemptyset(&sig);
	sigaddset(&sig, SIGALRM);

	struct sigaction handler = {.__sigaction_handler = {alarm_handler}, .sa_mask = sig};

	if (sigaction(SIGALRM, &handler, NULL) == -1)
	{
		perror("sigaction error");
		return ERROR_SIGACTION;
	}

	return 0;
}

int udp_client_socket_create(in_addr_t client_addr, in_port_t client_port)
{
	int client_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	
	if (client_socket == -1)
	{
		perror("socket error");
		return ERROR_SOCKET;
	}

	int reuse = 1;
	struct timeval time = {.tv_sec = 15};

	if ((setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) ||
		(setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,  &time, sizeof(time)) == -1))
	{
		perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = client_addr, .sin_port = client_port};

	if (bind(client_socket, (struct sockaddr*)&client, sizeof(struct sockaddr_in)) == -1)
	{
		perror("bind error");
		exit(EXIT_FAILURE);
	}

	return client_socket;
}

int broadcast_socket_create(in_addr_t client_addr)
{
	int client_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	
	if (client_socket == -1)
	{
		perror("socket error");
		return ERROR_SOCKET;
	}

	int reuse = 1;
	int broadcast = 1;
	struct timeval time = {.tv_sec = 15};

	if ((setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) ||
		(setsockopt(client_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
		(setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,  &time, sizeof(time)) == -1))
	{
		perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in client = {.sin_family = AF_INET, .sin_addr.s_addr = client_addr, .sin_port = 0};

	if (bind(client_socket, (struct sockaddr*)&client, sizeof(struct sockaddr_in)) == -1)
	{
		perror("bind error");
		exit(EXIT_FAILURE);
	}

	return client_socket;
}

int broadcast_search(in_addr_t client_addr, in_port_t broadcast_port)
{
	int client_socket = broadcast_socket_create(client_addr);

	if (client_socket < 0)
		return client_socket;
	
	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_BROADCAST, .sin_port = broadcast_port};
	socklen_t server_len = sizeof(server);

	static const char msg[] = "Where is server?";

	int sending = sendto(client_socket, msg, sizeof(msg), 0, (struct sockaddr*)&server, server_len);

	if (sending == -1)
	{
		perror("send error");
		return ERROR_SEND;
	}

	int timer = timer_creater();

	if (timer < 0)
	{
		close(client_socket);
		return timer;
	}

	char buffer[BUF_LEN] = {0};

	int delay = 1;
	alarm_wait = 0;
	alarm(delay);

	while (!alarm_wait)
	{   
		errno = 0;
		
		int read_size = recvfrom(client_socket, buffer, BUF_LEN - 1, 0, (struct sockaddr*)&server, &server_len);

		if (read_size == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
		{
			perror("recvfrom error");
			close(client_socket);
			return ERROR_RECVFROM;
		}

		buffer[read_size] = '\0';

		if (!errno && strcmp(buffer, "I'm here!") == 0)
		{
			printf("The IP address of the server %s was found\n", inet_ntoa(server.sin_addr));
		}
	}

	close(client_socket);
	return 0;
}

int tcp_server_socket_create(in_addr_t server_addr, in_port_t server_port)
{
	int socket_tcp = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_tcp == -1)
	{
		log_perror("socket error");
		return ERROR_SOCKET;
	}
	
	int reuse = 1;
	struct timeval time = {.tv_sec = 1200};

	if ((setsockopt(socket_tcp, SOL_SOCKET, SO_REUSEADDR, &reuse,    sizeof(int)) == -1) ||
		(setsockopt(socket_tcp, SOL_SOCKET, SO_RCVTIMEO,  &time, sizeof(struct timeval)) == -1))
	{
		log_perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in server_tcp = {.sin_family = AF_INET, .sin_addr.s_addr = server_addr, .sin_port = server_port};

	if (bind(socket_tcp, (struct sockaddr*)&server_tcp, sizeof(struct sockaddr_in)) == -1)
	{
		log_perror("bind error");
		return ERROR_BIND;
	}

	return socket_tcp;
}

int udp_server_socket_create(in_addr_t server_addr, in_port_t server_port)
{
	int socket_udp = socket(AF_INET, SOCK_DGRAM, 0);
	
	if (socket_udp == -1)
	{
		log_perror("socket error");
		return ERROR_SOCKET;
	}

	int reuse = 1;
	struct timeval time = {.tv_sec = 1200};

	if ((setsockopt(socket_udp, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) ||
		(setsockopt(socket_udp, SOL_SOCKET, SO_RCVTIMEO,  &time, sizeof(time)) == -1))
	{
		log_perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in server_udp = {.sin_family = AF_INET, .sin_addr.s_addr = server_addr, .sin_port = server_port};

	if (bind(socket_udp, (struct sockaddr*)&server_udp, sizeof(struct sockaddr_in)) == -1)
	{
		log_perror("bind error");
		return ERROR_BIND;
	}

	return socket_udp;
}

int client_operation(int protocol, in_addr_t address, char* username)
{
	if (protocol != SOCK_STREAM && protocol != SOCK_DGRAM)
	{
		printf("undefined protocol\n");
		exit(EXIT_FAILURE);
	}

//	struct rudp_header control = {};

	struct sockaddr_in server  = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = (protocol == SOCK_STREAM) ? htons(TCP_PORT) : htons(UDP_PORT)};
	struct sockaddr_in connected_address = {};

	struct termios term = {};
	struct termios oldt = {};

	if (tcgetattr(STDIN_FILENO, &oldt) == -1)
	{
		perror("tcsetattr error");
		exit(EXIT_FAILURE);
	}

	cfmakeraw(&term);

	if (tcsetattr(STDIN_FILENO, TCSANOW, &term) == -1)
	{
		perror("tcsetattr error");
		exit(EXIT_FAILURE);
	}

	char buffer[2<<20] = "";

	int socket = tcp_server_socket_create(INADDR_ANY, 0);

	if (socket < 0)
	{
		perror("tcp_socket error");
		return  ERROR_SOCKET;
	}

	if (connect(socket, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == -1)
	{
		perror("connect error");
		return ERROR_CONNECT;
	}

	int user_len = strlen(username);

	int send_name = sendto(socket, username, strlen(username), 0, (struct sockaddr*) &connected_address, sizeof(struct sockaddr_in));

	if (send_name == -1 && errno == EAGAIN)
	{
		log_perror("close connection");
		return ERROR_CLOSE_CONNECT;
	}

	else if (send_name == -1 || send_name != user_len)
	{
		printf("sendname=%d\totard=%d\n", send_name, user_len);
		log_perror("sendto error");
		return ERROR_SEND;
	}

	struct pollfd master[2] = {{.fd = socket, .events = POLL_IN}, {.fd = STDIN_FILENO, .events = POLL_IN}};

	int n_write = 0;
	int n_read  = 0;

	while (1)
	{
		int event = poll(master, 2, 100);
		if (event > 0)
		{
			if (master[0].revents == POLL_IN)
			{
				if (protocol == SOCK_STREAM)
                {
					n_read = recv(socket, buffer, sizeof(buffer), 0);

					if (n_read == -1)
					{
						perror("recv error");
						return -1;
					}

					if (n_read == 0)
					{
						if (close(socket) == -1)
						{
							perror("close error");
							return ERROR_CLOSE;
						}

						break;
					}
				}
/*
				else
				{

				}
*/
				n_write = write(STDOUT_FILENO, buffer, n_read);

				if (n_write == -1)
				{
					perror("write error");
					return ERROR_WRITE;
				}
			}

			if (master[1].revents == POLL_IN)
			{
				n_read = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

				if (n_read == -1)
				{
					perror("read error");
					return -1;
				}

				if (*buffer == 4) 
				{
					if (close(socket) == -1)
					{
						perror("close error");
						return ERROR_CLOSE;
					}

					break;
				}

				if (protocol == SOCK_STREAM)
				{
					n_write = sendto(socket, buffer, n_read, 0, NULL, sizeof(struct sockaddr_in));

					if (n_write == -1 && errno == EAGAIN)
					{
						log_perror("close connection");
						return ERROR_CLOSE_CONNECT;
					}

					else if (n_write == -1 || n_write != strlen(username));
					{
						log_perror("sendto error");
						return ERROR_SEND;
					}
				}
/*
				else
				{
					
				}
*/				
			}
		}

		else if (event == 0)
			continue;
		
		else
			break;
	}

	if (tcsetattr(STDIN_FILENO, TCSANOW, &oldt) == -1)
	{
		perror("tcsetattr error");
		exit(EXIT_FAILURE);
	}

	return 0;
}