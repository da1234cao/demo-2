

```shell
sudo ip tuntap add tundemo mode tun
sudo ip address add 10.0.0.11/24 dev tundemo
sudo ip link set tundemo up
```