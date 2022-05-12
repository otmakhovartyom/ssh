#include "../headers/ssh_lib.h"
#include "../headers/server.h"

int main(int argc, char *argv[])
{
	// if (argc != 3)
	// {
	// 	printf("Please, run the program in the following format: \n"               "./server <host> <port>\n");
	// 	return ERROR_INVALID_ARGC;
	// }

	int pid_broadcast = fork();

	if (pid_broadcast == -1)
	{
		log_perror("broadcast_fork error");
		return ERROR_FORK;
	}

	if (pid_broadcast == 0)
	{
		log_info("broadcast has been started");
		
		int broadcast_process = broadcast_waiting(INADDR_ANY, htons(BROADCAST_PORT));
		log_info("broadcast has been stopped, error %d: %s", broadcast_process, strerror(errno));

		return broadcast_process;
	}

	int pid_tcp = fork();

	if (pid_tcp == -1)
	{
		log_perror("tcp_fork error");
		return ERROR_FORK;
	}

	if (pid_tcp == 0)
	{
		log_info("tcp-server has been started");
		
		int tcp_process = server_operation(SOCK_STREAM, INADDR_ANY, htons(TCP_PORT));
		log_info("tcp-server has been stopped, error %d: %s", tcp_process, strerror(errno));

		return tcp_process;
	}

    int pid_udp = fork();

	if (pid_udp == -1)
	{
		log_perror("udp_fork error");
		return ERROR_FORK;
	}

	if (pid_udp == 0)
	{
		log_info("udp-server has been started");
		
		int udp_process = server_operation(SOCK_DGRAM, INADDR_ANY, htons(UDP_PORT));
		log_info("udp-server has been stopped, error %d: %s", udp_process, strerror(errno));

		return udp_process;
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

int broadcast_socket_create(in_addr_t address, in_port_t broadcast_port)
{
	int server_socket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	
	if (server_socket == -1)
	{
		log_perror("socket error");
		return ERROR_SOCKET;
	}

	int reuse = 1;
	int broadcast = 1;
	struct timeval time = {.tv_sec = 15};

	if ((setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) == -1) ||
		(setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(int)) == -1) ||
		(setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO,  &time, sizeof(time)) == -1))
	{
		log_perror("setsockopt error");
		return ERROR_SETSOCKOPT;
	}

	struct sockaddr_in server = {.sin_family = AF_INET, .sin_addr.s_addr = address, .sin_port = broadcast_port};

	if (bind(server_socket, (struct sockaddr*)&server, sizeof(struct sockaddr_in)) == -1)
	{
		log_perror("bind error");
		close(server_socket);
		return ERROR_BIND;
	}

	return server_socket;
}

int broadcast_waiting(in_addr_t serv_addr, in_port_t broadcast_port)
{
	int server_socket = broadcast_socket_create(serv_addr, broadcast_port);

	if (server_socket < 0)
		return server_socket;

	struct sockaddr_in client = {};
	socklen_t client_len = sizeof(client);

	char buffer[BUF_LEN] = "";

	log_info("broadcast message is being received\n");

	while (1)
	{
		int recsize = recvfrom(server_socket, buffer, BUF_LEN - 1, 0, (struct sockaddr*)&client, &client_len);
		
		if (recsize == -1)
		{
			if (errno == EAGAIN);
				continue;
			
			log_perror("recvfrom error");
			return ERROR_RECVFROM;
		}

		buffer[recsize] = '\0';

		log_info("broadcast from %s:%d :: %s\n", inet_ntoa(((struct sockaddr_in*)&client)->sin_addr), ntohs(((struct sockaddr_in*)&client)->sin_port), buffer);

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
	}

	close(server_socket);
	log_info("broadcast socket close\n");
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

int tcp_acceptance(int socket, struct sockaddr_in *address)
{
	socklen_t len = sizeof(struct sockaddr_in);

	int accepting_socket = accept(socket, (struct sockaddr*)address, &len);

	if (accepting_socket == -1)
	{
		log_perror("accept error");
		return ERROR_ACCEPT;
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		log_perror("fork error");
		return ERROR_FORK;
	}

	if (!pid)
	{
		if (close(socket) == -1)
		{
			log_perror("close error");
			return ERROR_CLOSE;
		}

		return accepting_socket;
	}
}

int udp_acceptance(int socket, struct sockaddr_in *address)
{
	socklen_t len = sizeof(struct sockaddr_in);

	int accepting_socket = accept(socket, (struct sockaddr*)address, &len);

	if (accepting_socket == -1)
	{
		log_perror("accept error");
		return ERROR_ACCEPT;
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		log_perror("fork error");
		return ERROR_FORK;
	}

	if (!pid)
	{
		if (close(socket) == -1)
		{
			log_perror("close error");
			return ERROR_CLOSE;
		}

		return accepting_socket;
	}
}

int server_operation(int protocol, in_addr_t address, in_port_t port)
{
	if (protocol == SOCK_STREAM)
	{
		int socket = tcp_server_socket_create(address, port);

		if (socket < 0)
			return socket;
		
		log_info("tcp_socket: port %d", ntohs(port));

		int listening = listen(socket, 5);

		if (listening == -1)
		{
			log_perror("listen error");
			return ERROR_LISTEN;
		}

		log_info("tcp_socket is listening: port %d", ntohs(port));

		struct sockaddr_in client = {};

		while (1)
		{
			int accepting_socket = tcp_acceptance(socket, &client);

			if (accepting_socket < 0)
				return accepting_socket;
			
			if (accepting_socket > 0)
			{
				char buf[2 << 20] = "";
				char slave[BUF_LEN] = "";

				int master_fd = 0;
				
				char username[BUF_LEN] = "";

				int auth = recv(accepting_socket, username, BUF_LEN - 1, 0);

				if (auth < 0)
				{
					if (errno == EAGAIN)
						return ERROR_CLOSE_CONNECT;
					
					return ERROR_RECVFROM;
				}

				pid_t pid = pty_fork(&master_fd, slave, BUF_LEN, NULL, NULL);

				if (pid < 0)
				{
					log_perror("pty_fork error");
					return ERROR_FORK;
				}

				if (pid == 0)
				{
					auth = login_into_user(username);

					if (auth == -1)
					{
						if (close(socket) == -1)
							return ERROR_CLOSE;
						
						return -1;
					}

					auth = set_id(username);

					if (auth == -1)
					{
						log_perror("set_id error");
						
						if (close(socket) == -1)
							return ERROR_CLOSE;
						
						return -1;
					}

					char* bash_argv[] = {"bash", NULL};
					execvp("bash", bash_argv);

					return -1;
				}

				struct pollfd master[2] = {{.fd = master_fd, .events = POLL_IN}, {.fd = accepting_socket, .events = POLL_IN}};

				size_t n_write = 0;
				size_t n_read  = 0;

				while (1)
				{
					int event = poll(master, 2, 100);
					if (event > 0)
					{
						if (master[0].revents == POLL_IN)
						{
							n_read = read(master_fd, buf, sizeof(buf));

							if (n_read == -1)
							{
								log_perror("read error");
								return ERROR_READ;
							}

							log_info("server read errno = %d", errno);

							n_write = sendto(accepting_socket, buf, n_read, 0, NULL, sizeof(struct sockaddr_in));

							if (n_write == -1 && errno == EAGAIN)
							{
								log_perror("close connection");
								return ERROR_CLOSE_CONNECT;
							}

							else if (n_write == -1 || n_write != n_read)
							{
								log_perror("sendto error");
								return ERROR_SEND;
							}

							log_info("server send errno = %d", errno);
						}

						if (master[1].revents == POLL_IN)
						{
							n_read = recv(accepting_socket, buf, sizeof(buf), 0);

							if (n_read == -1)
							{
								log_perror("rudp_send(SOCK_STREAM)");
								return -1;
							}

							buf[n_read] = '\0';

							log_info("server recv %ld \"%s\" errno = %d", n_read, buf, errno);

							if (!n_read)
							{
								log_info("client was shutdowned, errno = %d", errno);

								if (close(accepting_socket) == -1)
								{
									log_perror("close error");
									return ERROR_CLOSE;
								}

								return 1;
							}

							log_info("server recv errno = %d", errno);

							n_write = write(master_fd, buf, n_read);

							if (n_write == -1)
							{
								log_perror("write error");
								return ERROR_WRITE;
							}

							log_info("server write errno = %d", errno);
						}
					}

					else if (event == 0) continue;

					else
						break;
					
				}

                return 0;
            }
        }

        return 0;
	}

//	if (protocol == SOCK_DGRAM)
//	{
//
//	}
}