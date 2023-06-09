CC = gcc
LD = ld

CFLAGS += -Wall -Wextra -I ./ -Og

TARGET = akarivpn-client akarivpn-server
OBJS = akarivpn.o tunnel.o tuntap.o netif.o fdb.o
COBJS = client/main.o
SOBJS = server/main.o

.PHONY: client server clean

client: $(OBJS) $(COBJS)
	$(CC) $(CFLAGS) -o akarivpn-client $(OBJS) $(COBJS)

server: $(OBJS) $(SOBJS)
	$(CC) $(CFLAGS) -o akarivpn-server $(OBJS) $(SOBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.out $(OBJS) $(TARGET) $(COBJS) $(SOBJS)
