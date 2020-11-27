TARGET_CLIENT = client.out
TARGET_SERVER = server.out

# XXX: Don't forget backslash at the end of any line except the last one
HDRS = \
	   project/include

SRCS_CLIENT = \
       project/src/client_main.c \
       project/src/client.c

SRCS_SERVER = \
       project/src/server.c \
       project/src/server_main.c

.PHONY: all clean

all: client server

client: $(SRCS_CLIENT)
	$(CC) -Wall -Wextra -Werror $(addprefix -I,$(HDRS)) -o $(TARGET_CLIENT) $(CFLAGS) $(SRCS_CLIENT)

server: $(SRCS_SERVER)
	$(CC) -Wall -Wextra -Werror $(addprefix -I,$(HDRS)) -o $(TARGET_SERVER) $(CFLAGS) $(SRCS_SERVER)

clean:
	rm -rf $(TARGET_CLIENT) $(TARGET_SERVER)






