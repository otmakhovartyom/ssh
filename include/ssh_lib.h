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

#define BUF_LEN 1000

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
};

int print_time();
int init_log(char* path);
void print_log(char* str, ...);
void printf_fd(int fd, char* str, ...);
#define log(fmt, ...) print_log("%s:%d " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define log_info(fmt, ...) log("[INFO] " fmt, ##__VA_ARGS__)
#define log_error(fmt, ...) log("[ERROR] " fmt, ##__VA_ARGS__)
#define log_perror(fmt, ...) log (fmt " (errno = %d): %s ", errno, strerror(errno), ##__VA_ARGS__)

void* log_calloc(size_t nmemb, size_t sz);
int send_file(char* input, int client_fd, int size, int is_udp, struct sockaddr_in* server, int key);
int get_file(char* input, int client_fd, int size, _Bool is_udp, struct sockaddr_in* server, int key);
