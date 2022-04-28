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

//	printf("%s\n", argv[i]);

	while (i < argc)
	{
		if (strcmp(argv[i], "--broadcast") == 0 || (strcmp(argv[i], "-b")) == 0)
		{
//			printf("broadcast\n");
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
//			socklen_t server_len = sizeof(server);

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
			printf("Server %s:%u\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));
		}
	}

	close(client_socket);
	return 0;
}