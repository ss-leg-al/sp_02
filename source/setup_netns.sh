#!/bin/bash

set -e

NETNS_NAME="sslab-ns"
VETH_HOST="veth-host"
VETH_NS="veth-ns"
HOST_IP="10.0.1.1/24"
NS_IP="10.0.1.2/24"
NS_DEFAULT_GW="10.0.1.1"

# 1. Create the netns
ip netns add $NETNS_NAME

# 2. Create veth pair
ip link add $VETH_HOST type veth peer name $VETH_NS

# 3. Move one end into the netns
ip link set $VETH_NS netns $NETNS_NAME

# 4. Assign IPs
ip addr add $HOST_IP dev $VETH_HOST
ip netns exec $NETNS_NAME ip addr add $NS_IP dev $VETH_NS

# 5. Bring up interfaces
ip link set $VETH_HOST up
ip netns exec $NETNS_NAME ip link set $VETH_NS up
ip netns exec $NETNS_NAME ip link set lo up

# 6. Setup default route in netns
ip netns exec $NETNS_NAME ip route add default via $NS_DEFAULT_GW

# 7. Enable IP forwarding on host
sysctl -w net.ipv4.ip_forward=1

echo "[+] NetNS '$NETNS_NAME' is set up with:"
echo "    Host side: $VETH_HOST → 10.0.1.1"
echo "    NetNS side: $VETH_NS → 10.0.1.2"

