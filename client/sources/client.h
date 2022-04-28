int alarm_wait = 0;

void alarm_handler(int signum);
void help_message();
int timer_creater();
int udp_client_socket_create(in_addr_t client_addr, in_port_t client_port);
int broadcast_socket_create(in_addr_t client_addr);
int broadcast_search(in_addr_t client_addr, in_port_t broadcast_port);

