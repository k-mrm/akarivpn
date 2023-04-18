sudo ip netns add host1
sudo ip netns add host2
sudo ip netns exec host1 ip link set lo up
sudo ip netns exec host2 ip link set lo up
sudo ip link add name veth1 type veth peer name br-veth1
sudo ip link add name host1 type veth peer name br-host1
sudo ip link add name host2 type veth peer name br-host2
sudo ip link set host1 netns host1
sudo ip link set host2 netns host2
sudo ip link add br0 type bridge
sudo ip link set dev br-veth1 master br0
sudo ip link set dev br-host1 master br0
sudo ip link set dev br-host2 master br0
sudo ip addr add 10.0.0.100/24 dev veth1
sudo ip netns exec host1 ip addr add 10.0.0.1/24 dev host1
sudo ip netns exec host2 ip addr add 10.0.0.2/24 dev host2
sudo ip netns exec host1 ip link set host1 up
sudo ip netns exec host2 ip link set host2 up
sudo ip link set veth1 up
sudo ip link set br-veth1 up
sudo ip link set br-host1 up
sudo ip link set br-host2 up
sudo ip link set br0 up
