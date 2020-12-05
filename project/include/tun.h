#ifndef PROJECT_INCLUDE_TUN_H_
#define PROJECT_INCLUDE_TUN_H_

int tun_alloc(char* dev, int flags);
int cread(int fd, char* buf, int n);
int cwrite(int fd, char* buf, int n);
int read_n(int fd, char* buf, int n);
void do_debug(char* msg, ...);
void my_err(char* msg, ...);
void usage(void);
int main_tun(int argc, char* argv[]);

int create_client_tun(char* if_name, int port, char* network);
int event_anticipation(int tun_socket, int serv_socket);

#endif  // PROJECT_INCLUDE_SERVER_H_
