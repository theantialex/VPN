#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>

#include "tun.h"

/* buffer for reading from tun/tap interface, must be >= 1500 */
#define BUFSIZE 2000   
#define CLIENT 0
#define SERVER 1
#define PORT 55555

/* some common lengths */
#define IP_HDR_LEN 20
#define ETH_HDR_LEN 14
#define ARP_PKT_LEN 28

int debug;
char *progname;

/**************************************************************************
 * tun_alloc: allocates or reconnects to a tun/tap device. The caller     *
 *            needs to reserve enough space in *dev.                      *
 **************************************************************************/
int tun_alloc(char *dev, int flags) {
  struct ifreq ifr;
  int fd, err;


  if( (fd = open("/dev/net/tun", O_RDWR)) < 0 ) {
    goto error;
  }

  if (memset(&ifr, 0, sizeof(ifr)) == NULL) {
    close(fd);
    goto error;
  }

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    close(fd);
    goto error;
  }

  // strcpy(dev, ifr.ifr_name);

  return fd;

error:
  printf("Error occured while allocing tun: %s\n", strerror(errno));
	return FAILURE;
}

/**************************************************************************
 * cread: read routine that checks for errors and exits if an error is    *
 *        returned.                                                       *
 **************************************************************************/
// int cread(int fd, char *buf, int n){
  
//   int nread;

//   if((nread=read(fd, buf, n))<0){
//     perror("Reading data");
//     exit(1);
//   }
//   return nread;
// }

/**************************************************************************
 * cwrite: write routine that checks for errors and exits if an error is  *
 *         returned.                                                      *
 **************************************************************************/
// int cwrite(int fd, char *buf, int n){
  
//   int nwrite;

//   if((nwrite=write(fd, buf, n))<0){
//     perror("Writing data");
//     exit(1);
//   }
//   return nwrite;
// }

/**************************************************************************
 * read_n: ensures we read exactly n bytes, and puts those into "buf".    *
 *         (unless EOF, of course)                                        *
 **************************************************************************/
// int read_n(int fd, char *buf, int n) {

//   int nread, left = n;

//   while(left > 0) {
//     if ((nread = cread(fd, buf, left))==0){
//       return 0 ;      
//     }else {
//       left -= nread;
//       buf += nread;
//     }
//   }
//   return n;  
// }

/**************************************************************************
 * do_debug: prints debugging stuff (doh!)                                *
 **************************************************************************/
void do_debug(char *msg, ...){
  
  va_list argp;
  
  if (debug){
    va_start(argp, msg);
    vfprintf(stderr, msg, argp);
    va_end(argp);
  }
}

/**************************************************************************
 * my_err: prints custom error messages on stderr.                        *
 **************************************************************************/
void my_err(char *msg, ...) {

  va_list argp;
  
  va_start(argp, msg);
  vfprintf(stderr, msg, argp);
  va_end(argp);
}

/**************************************************************************
 * usage: prints usage and exits.                                         *
 **************************************************************************/
void usage(void) {
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "%s -i <ifacename> [-s|-c <serverIP>] [-p <port>] [-u|-a] [-d]\n", progname);
  fprintf(stderr, "%s -h\n", progname);
  fprintf(stderr, "\n");
  fprintf(stderr, "-i <ifacename>: Name of interface to use (mandatory)\n");
  fprintf(stderr, "-s|-c <serverIP>: run in server mode (-s), or specify server address (-c <serverIP>) (mandatory)\n");
  fprintf(stderr, "-p <port>: port to listen on (if run in server mode) or to connect to (in client mode), default 55555\n");
  fprintf(stderr, "-u|-a: use TUN (-u, default) or TAP (-a)\n");
  fprintf(stderr, "-d: outputs debug information while running\n");
  fprintf(stderr, "-h: prints this help text\n");
  exit(1);
}

int create_client_tun(char* if_name) {
  int tun_fd;
  int sock_fd;
  struct sockaddr_in address;
  int optval = 1;
  debug = 1;

  if ( (tun_fd = tun_alloc(if_name, IFF_TUN)) < 0 ) {
    my_err("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }

  do_debug("Successfully connected to interface %s\n", if_name);

  if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }

  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
      perror("setsockopt()");
      exit(1);
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(0);
  if (bind(sock_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
      perror("bind()");
      exit(1);
  }

  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  if (getsockname(sock_fd, (struct sockaddr *)&sin, &len) != -1) {
    printf("port number %d\n", ntohs(sin.sin_port));
  }
    
  if (listen(sock_fd, 5) < 0) {
      perror("listen()");
      exit(1);
  }

  return sock_fd;
}

int create_server_tun(char* if_name, hserver_config_t* server_param) {
  int tun_fd;
  int sock_fd;
  struct sockaddr_in address;
  int optval = 1;
  debug = 1;

  if ( (tun_fd = tun_alloc(if_name, IFF_TUN)) < 0 ) {
    my_err("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }

  do_debug("Successfully connected to interface %s\n", if_name);

  if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket()");
    exit(1);
  }

  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval)) < 0) {
      perror("setsockopt()");
      exit(1);
  }

  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(server_param->address);
  address.sin_port = htons(server_param->port);
  
  if (connect(sock_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
      perror("connect()");
      exit(1);
  }

  return sock_fd;
}

