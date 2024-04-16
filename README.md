# akarivpn

a toy L2 VPN with tap interface

## Warning
buggy

## Usage
- server:
```
$ make server
$ echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward
$ sudo iptables -t nat -A POSTROUTING -s 192.168.1.0/24 -o <network interface> -j MASQUERADE
$ sudo ./akarivpn-server
```

- client:
```
$ make client
$ sudo ./akarivpn-client -i 192.168.1.xx -s <server-ip>
```
(*xx* must not be 1)

## Quick Demo
1. Prepare environment (network namespace)

```
$ sh demons.sh
```
2. Start server
3. Start client 1 in netns 1

```
$ sudo ip netns exec host1 bash
# akarivpn-client -i 192.168.1.10 -s 10.0.0.100
```
4. Start client 2 in netns 2

```
$ sudo ip netns exec host2 bash
# akarivpn-client -i 192.168.1.20 -s 10.0.0.100
```

### ping each other
- in netns 1
```
$ ping 192.168.1.1     # ping to vpn server
$ ping 192.168.1.20    # ping to client 2
```

 - in netns 2
```
$ ping 192.168.1.1     # ping to vpn server
$ ping 192.168.1.10    # ping to client 1
```

### GO INTERNET
- in netns
```
$ sudo ip route add default via 192.168.1.1      # set gw to vpn server
$ ping 1.1.1.1                                   # :)
```
