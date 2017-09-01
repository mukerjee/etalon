#!/bin/bash


# OVS patch ports between the bridges
function create_ovs_chain_patch {
  local BRIDGE_PREFIX=$1
  local NUM_OVS=$2

  # create the switches
  for I in $(seq 1 $NUM_OVS)
  do
    BNAMEI="$BRIDGE_PREFIX-$I"
    ovs-vsctl add-br $BNAMEI
    if [ $I -gt 1 ]
    then
      let K=I-1
      BNAMEK="$BRIDGE_PREFIX-$K"
      PNAMEI="patch-CONNECT-$I$K"
      PNAMEK="patch-CONNECT-$K$I"
      # need to connect this bridge to the previous bridge
      ovs-vsctl add-port $BNAMEI $PNAMEI -- set Interface $PNAMEI type=patch options:peer=$PNAMEK
      ovs-vsctl add-port $BNAMEK $PNAMEK -- set Interface $PNAMEK type=patch options:peer=$PNAMEI
    fi
  done
}

# Linux veth pairs between the bridges
function create_ovs_chain_veth {
    local BRIDGE_PREFIX=$1
    local NUM_OVS=$2
    local VETH_PREFIX="ovschain"

    # create the switches
    # echo "****** Add switch ******"
    local I
    for I in $(seq 1 $NUM_OVS)
    do
        BNAMEI="${BRIDGE_PREFIX}-${I}"
        # echo "add switch ${BNAMEI}"
        ovs-vsctl add-br $BNAMEI

        if [ $I -gt 1 ]
        then
            let K=I-1
            BNAMEK="${BRIDGE_PREFIX}-${K}"
            PNAMEI="${VETH_PREFIX}-${I}${K}"
            PNAMEK="${VETH_PREFIX}-${K}${I}"

            # need to connect this bridge to the previous bridge
            # echo "add port ${PNAMEI} and ${PNAMEK} on switch ${BNAMEI}"
            ip link add $PNAMEI type veth peer name $PNAMEK
            ovs-vsctl add-port $BNAMEI $PNAMEI
            ovs-vsctl add-port $BNAMEK $PNAMEK

            ip link set dev $PNAMEI up
            ip link set dev $PNAMEK up

            # do not tc on ovs
            # add tc to switch
            #tc qdisc add dev $PNAMEI root handle 1: htb default 12 
            #tc class add dev $PNAMEI parent 1:1 classid 1:12 htb rate 100mbit burst 15k
            #tc qdisc add dev $PNAMEI parent 1:12 netem delay 10ms
            # FIXME: use same handle and parent id OK ???
            #tc qdisc add dev $PNAMEK root handle 1: htb default 12 
            #tc class add dev $PNAMEK parent 1:1 classid 1:12 htb rate 100mbit burst 15k
            #tc qdisc add dev $PNAMEK parent 1:12 netem delay 10ms

        fi
    done
}

create_host() {
    local HOST_ID=$1
    local SW_ID=$2
    local BRIDGE_PREFIX=$3

    NS="ns${HOST_ID}"
    SW="${BRIDGE_PREFIX}-${SW_ID}"
    HOST_PORT="tap${HOST_ID}"
    SW_PORT="ovs-tap${HOST_ID}"

    # echo "****** Add host *******"
    # add network namespace
    # echo "add namespace ${NS}"
    ip netns add $NS

    # create veth pair (tap_id, ovs-tap_id)
    # echo "add link between ports ${HOST_PORT} <-> ${SW_PORT}"
    ip link add $HOST_PORT type veth peer name $SW_PORT

    # attach tap_id to ns_id
    # echo "attach ${HOST_PORT} to ${NS}"
    ip link set $HOST_PORT netns $NS


    # attach ovs-tap_id to switch/bridge
    # echo "attach ${SW_PORT} to ${SW}"
    ovs-vsctl add-port $SW $SW_PORT

    # bring veth ports up
    ip netns exec $NS ip link set dev $HOST_PORT up
    ip link set dev $SW_PORT up

    # assign IP address to host
    # echo "set IP address 10.1.1.${HOST_ID}/24 to port ${HOST_PORT}"
    ip netns exec $NS ip addr add 10.1.1.${HOST_ID}/24 dev $HOST_PORT


    # add tc to host
    ip netns exec $NS tc qdisc add dev $HOST_PORT root handle 1: htb default 12 
    ip netns exec $NS tc class add dev $HOST_PORT parent 1:1 classid 1:12 htb rate 100mbit burst 15k
    ip netns exec $NS tc qdisc add dev $HOST_PORT parent 1:12 netem delay 1ms
    ip netns exec $NS tc qdisc show dev $HOST_PORT

    # FIXME do we need to tc on port pair?
    # add tc to switch
    tc qdisc add dev $SW_PORT root handle 1: htb default 12 
    tc class add dev $SW_PORT parent 1:1 classid 1:12 htb rate 100mbit burst 15k
    tc qdisc add dev $SW_PORT parent 1:12 netem delay 10ms

}

bandwidth_test() {
    
    SERVER_ID=$1
    CLIENT_ID=$2
    SERVER_NS="ns${SERVER_ID}"
    CLIENT_NS="ns${CLIENT_ID}"
    
    sudo killall iperf
    ip netns exec $SERVER_NS iperf -s -f m &
    ip netns exec $CLIENT_NS iperf -c 10.1.1.${SERVER_ID} -f m -t 10 #-r

}

delete_host() {
    HOST_ID=$1
    SW_ID=$2
    BRIDGE_PREFIX=$3

    NS="ns${HOST_ID}"
    SW="${BRIDGE_PREFIX}-${SW_ID}"
    HOST_PORT="tap${HOST_ID}"
    SW_PORT="ovs-tap${HOST_ID}" 

    # echo "****** Delete host ******"
    # bring ports/link down
    ip netns exec $NS ip link set dev $HOST_PORT down
    ip link set dev $SW_PORT down

    # delete host link
    # echo "delete port ${HOST_PORT} on host"
    ip netns exec $NS ip link delete $HOST_PORT


    # delete switch link
    # echo "delete port ${SW_PORT} on switch ${SW}"
    ovs-vsctl del-port $SW $SW_PORT
    ip link delete $SW_PORT

    # delete network namespace
    # echo "delete namespace ${NS}"
    ip netns delete $NS
    
}

delete_switches() {
    local BRIDGE_PREFIX=$1
    local NUMOFOVS=$2
    local VETH_PREFIX="ovschain"
    # echo "*** Deleting interfaces"

    # create the switches
    # echo "****** Delete switch ******"
    local I
    for I in $(seq 1 $NUMOFOVS)
    do
        BNAMEI="${BRIDGE_PREFIX}-${I}"

        if [ $I -lt $NUMOFOVS ]
        then
            let K=I+1
            BNAMEK="${BRIDGE_PREFIX}-${K}"
            PNAMEI="${VETH_PREFIX}-${I}${K}"
            PNAMEK="${VETH_PREFIX}-${K}${I}"

            # bring down ports
            ip link set dev $PNAMEI down
            ip link set dev $PNAMEK down
            
            # echo "delete link between ports ${PNAMEI} and ${PNAMEK}"
            ovs-vsctl del-port $BNAMEI $PNAMEI
            ovs-vsctl del-port $BNAMEK $PNAMEK
            ip link delete $PNAMEI
            ip link delete $PNAMEK
        fi
        # echo "delete switch ${BNAMEI}"
        ovs-vsctl del-br $BNAMEI

    done
}


main () {

    BRIDGE_PREFIX=$1
    NUM_OVS=$2
    LOG="${NUM_OVS}chain.out"
    exec > >(tee $LOG)
    # exec 2>&1

    # create switch chain
    create_ovs_chain_veth $BRIDGE_PREFIX $NUM_OVS

    # create hosts
    create_host 1 1 $BRIDGE_PREFIX
    create_host 2 $NUM_OVS $BRIDGE_PREFIX

    # iperf test
    bandwidth_test 1 2

    # delete hosts' ports and delete network namespace
    delete_host 1 1 $BRIDGE_PREFIX
    delete_host 2 $NUM_OVS $BRIDGE_PREFIX

    # delete switches/bridges
    delete_switches $BRIDGE_PREFIX $NUM_OVS

}

after_fail() {
    delete_switches $BRIDGE_PREFIX $NUM_OVS
}

BRIDGE_PREFIX="ovswitch"
NUM_OVS="1 2 4 8 16 32 64 96 128 256 384 512 640 768 896 1024"

# this program fails if number of switch is 1024
for i in $NUM_OVS; do
    echo "Test with ${i} switches"
    main $BRIDGE_PREFIX $i
done

