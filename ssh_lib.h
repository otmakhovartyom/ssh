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