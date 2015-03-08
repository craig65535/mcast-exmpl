# mcast-exmpl

In one terminal:

```
$ make
$ sudo insmod mcast-exmpl.ko
$ cd client
$ make
$ ./client
genl ctrl msg
Family ID: 26
Mcast group ID: 4
```
(IDs may be different)

In another terminal, run a command that will do a TCP connect. mcast-exmpl hooks connects via a jprobe, so doing this will cause it to send a netlink multicast.
```
$ nc yahoo.com 80
^C
$
```

In the first terminal, you'll see a netlink multicast was received, *even if you did not run client as root.*
```
mcast-exmpl msg
SEND_NUM 55555
```
