TARGET_CLIENT = client.out
TARGET_SERVER = server.out

prefix=/opt/libevent
exec_prefix=${prefix}
libdir=${exec_prefix}/lib
includedir=${prefix}/include

# XXX: Don't forget backslash at the end of any line except the last one
HDRS = \
	   project/include

SRCS_CLIENT = \
       project/src/client_main.c \
       project/src/client.c \
       project/src/utils.c \
       project/src/ip_parser.c \
       project/src/tun.c

SRCS_SERVER = \
       project/src/server.c \
       project/src/server_main.c \
       project/src/utils.c

.PHONY: all clean

all: client server

client: $(SRCS_CLIENT)
	$(CC) -Wall -Wextra -Werror $(addprefix -I,$(HDRS)) -o $(TARGET_CLIENT) $(CFLAGS) $(SRCS_CLIENT) -lm -I${includedir} -levent

server: $(SRCS_SERVER)
	$(CC) -Wall -Wextra -Werror $(addprefix -I,$(HDRS)) -o $(TARGET_SERVER) $(CFLAGS) $(SRCS_SERVER)

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERVER)






