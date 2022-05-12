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

struct pam_conv my_conv = {misc_conv, NULL};

int login_into_user(char* username)
{
	pam_handle_t *pam = NULL;

	int ret = pam_start("my_ssh", username, &my_conv, &pam);

	if (ret != PAM_SUCCESS)
	{
		log_perror("start");
		printf("Failed pam_start\n");
		return -1;
	}

	ret = pam_authenticate(pam, 0);

	if (ret != PAM_SUCCESS)
	{
		log_perror("auth");
		printf("Incorrect password!\n");
		return -1;
	}

	ret = pam_acct_mgmt(pam, 0);

	if (ret != PAM_SUCCESS)
	{
		printf("User account expired!\n");
		return -1;
	}

	if (pam_end(pam, ret) != PAM_SUCCESS)
	{
		printf("Unable to pam_end()\n");
		return -1;
	}

	printf("Login succesfull\n");
	return 0;
}

int set_id(const char* username)
{
	struct passwd* info = getpwnam(username);

	if (!info)
	{
		log_perror("getpwnam()");
		return -1;
	}

	if (setgid(info->pw_gid) == -1)
	{
		log_perror("setgid()");
		return -1;
	}

	if (setuid(info->pw_uid) == -1)
	{
		log_perror("setuid()");
		return -1;
	}

	return 0;
}

int pty_master_open(char* slave_name, size_t slave_name_len)
{
	int master = posix_openpt(O_RDWR | O_NOCTTY);

	if (master == -1) 
	{
		log_perror("posix_openpt error");
        return -1;
	}

	if (grantpt(master) == -1)
	{
		log_perror("grantpt error");
		close(master);
		return -1;       
	}

	if (unlockpt(master) == -1)
	{
		log_perror("unlockpt error");
		close(master);
		return -1;
	}

	char* pathname = ptsname(master);

	if (pathname == NULL)
	{
		log_perror("ptsname error");
		close(master);
		return -1;
	}

	if (strlen(pathname) < slave_name_len)
		strncpy(slave_name, pathname, slave_name_len);
	
	else
	{
		close(master);
		errno = EOVERFLOW;
		log_perror("overflow slave name buffer");
		return -1;
	}

	return master;
}

pid_t pty_fork(int* master_fd, char* slave_name, size_t slave_name_len, const struct termios* slave_termios, const struct winsize* slave_winsize)
{
	char slave_name_buffer[BUF_LEN] = "";

	int mfd = pty_master_open(slave_name_buffer, BUF_LEN);

	if (mfd < 0)
		return mfd;
	
	if (slave_name != NULL)
	{
		if (strlen(slave_name_buffer) < slave_name_len)
			strncpy(slave_name, slave_name_buffer, slave_name_len);
		
		else
		{
			close(mfd);
			errno = EOVERFLOW;
			log_perror("overflow slave name buffer");
			return -1;
		}
	}

	pid_t pid = fork(); 

	if (pid == -1)
	{
		log_perror("fork error");
		close(mfd);
		return  ERROR_FORK;
	}

	if (pid) // parent
	{
		*master_fd = mfd;
		return pid;
	}   

	// child
	if (setsid() == -1)
	{
		log_perror("setsid error");
		exit(EXIT_FAILURE);
	}

	close(mfd);

	int slave_fd = open(slave_name_buffer, O_RDWR);
	
	if (slave_fd == -1)
	{
		log_perror("ERROR: pty_fork:open()");
		exit(EXIT_FAILURE);
	}

#ifdef TIOCSCTTY

	if (ioctl(slave_fd, TIOCSCTTY, 0) == -1)
	{
		log_perror("ioctl error");
		exit(EXIT_FAILURE);
	}

#endif // TIOCSTTY

	if (slave_termios != NULL)
	{
		if (tcsetattr(slave_fd, TCSANOW, slave_termios) == -1)
		{
			log_perror("tcsetattr error");
			exit(EXIT_FAILURE);
		}
	}

	if (slave_winsize != NULL)
	{
		if (ioctl(slave_fd, TIOCSWINSZ, slave_winsize) == -1)
		{
			perror("ERROR: pty_fork:icontl(TIOCSWINSZ)");
			exit(EXIT_FAILURE);
		}
	}

	if (dup2(slave_fd, STDIN_FILENO) != STDIN_FILENO)
	{
		perror("ERROR: pty_fork:dup2 - STDIN_FILENO");
		exit(EXIT_FAILURE);
	}

	if (dup2(slave_fd, STDOUT_FILENO) != STDOUT_FILENO)
	{
		perror("ERROR: pty_fork:dup2 - STDOUT_FILENO");
		exit(EXIT_FAILURE);
	}

	if (dup2(slave_fd, STDERR_FILENO) != STDERR_FILENO)
	{
		perror("ERROR: pty_fork:dup2 - STDERR_FILENO");
		exit(EXIT_FAILURE);
	}

	if (slave_fd > STDERR_FILENO)
		close(slave_fd);
	
	return 0;
}