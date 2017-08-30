term1: sudo mn --topo single,3 --mac --controller remote --switch ovsk

term2: dpctl add-flow tcp:127.0.0.1:6634 in_port=1,actions=output:2
term2: dpctl add-flow tcp:127.0.0.1:6634 in_port=2,actions=output:1

term1: h1 iperf3 -s &

term1: h2 iperf3 -c 10.0.0.1
