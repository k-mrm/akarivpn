# akarivpn

a toy L2 VPN with tap interface

## Warning
buggy

## Usage
- prepare environment

make netns:
```
$ sh demons.sh
```

- server:
```
$ make server
$ sudo ./akarivpn-server
```

- client:
```
$ make client
$ sudo ./akarivpn-client -i 192.168.1.xx -s <server-ip>
```
