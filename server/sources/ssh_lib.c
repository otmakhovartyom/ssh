#include "../headers/ssh_lib.h"

static int log_fd = -1;
#define LOG_SIZE (1 << 10)
static char buf_log[LOG_SIZE];

int print_time()
{
	struct tm* curtime;
	
	time_t t;
	t = time(NULL);
	
	if (t == -1)
		exit(1);
	
	curtime = localtime(&t);

	if (!curtime)
		exit(1);
	
	return dprintf(log_fd, "%02d.%02d.%d %02d:%02d:%02d ", curtime -> tm_mday, curtime -> tm_mon + 1, curtime -> tm_year + 1900, curtime -> tm_hour, curtime -> tm_min, curtime -> tm_sec);
}

int init_log(char* path)
{
	static char* default_path = "/home/otard/Progs/SSH_project/logfile.txt";

	log_fd = open(path ? path : default_path, O_CREAT | O_RDWR | O_APPEND, 0644);

	if (log_fd < 0)
		exit(1);
	
	print_time();
	return dprintf(log_fd, "Successful log init.\n");
}

void print_log(char* str, ...)
{
	va_list ap;
	va_start(ap, str);

	if (log_fd < 0)
		init_log(NULL);
	
	print_time();

	int ret = vsnprintf(buf_log, LOG_SIZE, str, ap);
	write(log_fd, buf_log, ret);

	va_end(ap);
}

void printf_fd(int fd, char* str, ...)
{
	va_list ap;
	va_start(ap, str);

	char buf_printf[10000];

	if (fd < 0)
	{
		printf("no such file\n");
		exit(1);
	}

	int ret = vsnprintf(buf_printf, 10000, str, ap);
	write(fd, buf_printf, ret);

	va_end(ap);
}

// dprintf -> snprintf from buffer for high speed

void* log_calloc(size_t nmemb, size_t sz)
{
	void *ret = calloc(nmemb, sz);

	if (ret == NULL)
	{
		log_perror("calloc error");
		exit(EXIT_FAILURE);
	}

	return ret;
}

int send_file(char* client_path, char* server_path, int client_fd, int is_udp, struct sockaddr_in* server)
{
//	char* path_name = strchr(input, ' ') + 1;
//	*strchr(path_name, ' ') = '\0';

	log_info("sending_file %s\n", client_path);

	unsigned path_size = sizeof(server_path);

	struct stat stats;
	int stat_error = stat(client_path, &stats);

	if (stat_error < 0)
	{
		printf("No such file\n");
		exit(EXIT_FAILURE);
	}

	int file_fd = open(client_path, O_RDONLY);

	if (file_fd < 0)
	{
		log_perror("open error");
		return ERROR_OPEN;
	}

	char *buf = (char*)log_calloc(stats.st_size, sizeof(char));

	read(file_fd, buf, stats.st_size);
//	log_info("file %s\n", buf);

	usleep(100000);

	unsigned message_size = stats.st_size;

	log_info("st_size = %u\n", message_size);

	if (sendto(client_fd, "sending", strlen("sending"), 0, (struct sockaddr*) server, sizeof(*server)) < 0)
	{
		log_perror("sendfile error");
		return ERROR_SEND;
	}

	usleep(100000);

	if (is_udp)
	{
		if (sendto(client_fd, (char*) &path_size, sizeof(unsigned), 0, (struct sockaddr*)server, sizeof(*server)) < 0)
		{
			perror("sendfile error");
			return ERROR_SEND;
		}

		usleep(10000);

		if (sendto(client_fd, &server_path, path_size, 0, (struct sockaddr*)server, sizeof(*server)) < 0)
		{
			perror("sendfile error");
			return ERROR_SEND;
		}

		usleep(10000);
		
		if (sendto(client_fd, (char*) &message_size, sizeof(unsigned), 0, (struct sockaddr*) server, sizeof(*server)) < 0)
		{
			log_perror("sendfile error");
			return ERROR_SEND;
		}

		usleep(100000);

		if (sendto(client_fd, buf, message_size, 0, (struct sockaddr*) server, sizeof(*server)) < 0)
		{
			log_perror("sendfile error");
			return ERROR_SEND;
		}

		log_info("was sended size = %d, mess = %s\n", message_size, buf);

//		if (sendto(client_fd, buf, message_size, 0, (struct sockaddr*) server, sizeof(*server)) < 0)
//		{
//			log_perror("sendfile error");
//			return ERROR_SEND;
//		}
	}

	else
	{
		if (write(client_fd, (char*) &message_size, sizeof(unsigned)) < 0)
		{
			log_perror("sendfile error");
			return ERROR_SEND;
		}

		usleep(100000);

		if (write(client_fd, buf, message_size) < 0)
		{
			log_perror("sendfile error");
			return ERROR_SEND;
		}
	}

	log_info("sending completed\n");
	
	free(buf);
	return 0;
}

int get_file(int client_fd, _Bool is_udp, struct sockaddr_in* server)
{
	log_info("getting a file\n");

	char *buffer = NULL;
	unsigned path_size = 0;
	unsigned message_size = 0;
	char *server_path = NULL;

	if (read(client_fd, (char*) &path_size, sizeof(unsigned)) < 0)
	{
		perror("sendfile error");
		return ERROR_SEND;
	}

	server_path = (char*) log_calloc(path_size, sizeof(char));

	if (read(client_fd, server_path, path_size) < 0)
	{
		log_perror("read error");
		return ERROR_READ;
	}

	if (read(client_fd, (char*) &message_size, sizeof(unsigned)) < 0)
	{
		log_perror("read error");
		return ERROR_READ;
	}

	log_info("sizeof(got message) = %u\n", message_size);

	buffer = (char*) log_calloc(message_size, sizeof(char));

	if (read(client_fd, buffer, message_size) < 0)
	{
		log_perror("read error");
		return ERROR_READ;
	}

	log_info("path_name = %s\n", server_path);

	int file_fd = open(server_path, O_CREAT | O_WRONLY | O_TRUNC, 0666);

	if (file_fd < 0)
	{
		log_perror("open error");
		return ERROR_OPEN;
	}

	if (write(file_fd, buffer, message_size) != message_size)
	{
		log_perror("write error");
		return -1;
	}

	log_info("getting completed\n");
	close(file_fd);
	free(buffer);
	return 0;
}