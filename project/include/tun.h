#ifndef PROJECT_INCLUDE_TUN_H_
#define PROJECT_INCLUDE_TUN_H_

int tun_alloc(char* dev, int flags);
// int cread(int fd, char* buf, int n);
// int cwrite(int fd, char* buf, int n);
// int read_n(int fd, char* buf, int n);
void do_debug(char* msg, ...);
void my_err(char* msg, ...);
void usage(void);

int create_client_tun(char* if_name, int port);

#endif  // PROJECT_INCLUDE_SERVER_H_
