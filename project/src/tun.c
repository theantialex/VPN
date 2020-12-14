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

/* some common lengths */
#define IP_HDR_LEN 20
#define ETH_HDR_LEN 14
#define ARP_PKT_LEN 28

int debug;
char *progname;

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

int create_client_tun(char* if_name, char* addr) {
  int tun_fd;
  debug = 1;
  char command[255];

  if ( (tun_fd = tun_alloc(if_name, IFF_TUN)) < 0 ) {
    my_err("Error connecting to tun/tap interface %s!\n", if_name);
    exit(1);
  }

  do_debug("Successfully connected to interface %s\n", if_name);

  strcpy(command, "ip a add ");
  strcat(command, addr);
  strcat(command, " dev ");
  strcat(command, if_name);
  if (system(command) != 0) {
    my_err("Error adding tun/tap interface %s!\n", if_name);
  }

  strcpy(command, "ip link set dev ");
  strcat(command, if_name);
  strcat(command, " up");
  if (system(command) != 0) {
    my_err("Error setting tun/tap interface %s up!\n", if_name);
  }

  return tun_fd;
}

int create_server_tun(char* if_name, hserver_config_t* server_param, char* addr) {
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

  char command[255];
  strcpy(command, "ip a add ");
  strcat(command, addr);
  strcat(command, " dev ");
  strcat(command, if_name);
  if (system(command) != 0) {
    my_err("Error adding tun/tap interface %s!\n", if_name);
  }

  strcpy(command, "ip link set dev ");
  strcat(command, if_name);
  strcat(command, " up");
  if (system(command) != 0) {
    my_err("Error setting tun/tap interface %s up!\n", if_name);
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

