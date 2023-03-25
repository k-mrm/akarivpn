CC = gcc
LD = ld

CFLAGS += -Wall -Wextra -I ./ -Og

TARGET = akarivpn-client akarivpn-server
OBJS = akarivpn.o netif.o
COBJS = client/main.o

.PHONY: client server clean

client: $(OBJS) $(COBJS)
	$(CC) $(CFLAGS) -o akarivpn-client $(OBJS) $(COBJS)

server: $(OBJS)
	$(CC) $(CFLAGS) -o akarivpn-server $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.out $(OBJS) $(TARGET)
