#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>


#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>

#include <pwd.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>

#include <termios.h>
#include <sys/ioctl.h>

#include <sys/poll.h>

#define BUF_LEN 1000
#define QUEUE_SIZE 100
#define BROADCAST_PORT 54321
#define SCP_PORT 44444
#define TCP_PORT 31243
#define UDP_PORT 29765

enum ERRORS
{
    ERROR_INVALID_ARGC    = -1,
    ERROR_INVALID_ARGV    = -2,
    ERROR_OPEN            = -3,
    ERROR_SOCKET          = -4,
    ERROR_CONNECT         = -5,
    ERROR_SEND            = -6,
    ERROR_INVALID_ADDRESS = -7,
    ERROR_READ            = -8,
    ERROR_BIND            = -9,
    ERROR_RECVFROM        = -10,
    ERROR_SETSOCKOPT      = -11,
    ERROR_SIGACTION       = -12,
    ERROR_ALLOCATE        = -13,
    ERROR_WRITE           = -14,
    ERROR_TIME            = -15,
    ERROR_LISTEN          = -16,
    ERROR_ACCEPT          = -17,
    ERROR_FORK            = -18,
    ERROR_CLOSE           = -19,
    ERROR_CLOSE_CONNECT   = -20,
    ERROR_POSIX_OPENPT    = -21
};

int print_time();
int init_log(char* path);
void print_log(char* str, ...);
void printf_fd(int fd, char* str, ...);
#define log(fmt, ...) print_log("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define log_info(fmt, ...) log("[INFO] " fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log("[ERROR] " fmt, ##__VA_ARGS__)
#define log_perror(fmt, ...) log (fmt " (errno = %d): %s \n", errno, strerror(errno), ##__VA_ARGS__)

void* log_calloc(size_t nmemb, size_t sz);
int send_file(char* client_path, char* server_path, int client_fd, int is_udp, struct sockaddr_in* server);
int get_file(int client_fd, _Bool is_udp, struct sockaddr_in* server);

int login_into_user(char *username);
int set_id(const char* username);

int pty_master_open(char* slave_name, size_t slave_name_len);
pid_t pty_fork(int* master_fd, char* slave_name, size_t slave_name_len, const struct termios* slave_termios, const struct winsize* slave_winsize);