# TCP-project

The objective of this project is to layout a protocol that uses UDP as a transport protocol with
characteristic of TCP on top of that, to reach that goal a header is designed as prefix of the data to be
transmitted, the protocol has to have features as Error and Flow control as well as interaction in case of
Congestion over the network. The final purpose is to make sure the data gets to the server in the proper
order so assuring the delivery of the same.



In Order to compile the client and server code do the following:

gcc -o serverf server.c -lm
gcc -o clientf client.c -lm

for Network simulation purpose:
Use the Traffic Shaper
sudo tc qdisc add dev lo root netem delay 10 ms reorder 25% 50% 
