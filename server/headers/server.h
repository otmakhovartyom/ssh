int broadcast_socket_create(in_addr_t address, in_port_t broadcast_port);
int broadcast_waiting(in_addr_t serv_addr, in_port_t broadcast_port);
int tcp_server_socket_create(in_addr_t server_addr, in_port_t server_port);
int udp_server_socket_create(in_addr_t server_addr, in_port_t server_port);
int tcp_acceptance(int socket, struct sockaddr_in *address);
int udp_acceptance(int socket, struct sockaddr_in *address);
int server_operation(int protocol, in_addr_t address, in_port_t port);