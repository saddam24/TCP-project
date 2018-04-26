In Order to compile the client and server code do the following:

gcc -o serverf server.c -lm
gcc -o clientf client.c -lm

for Network simulation purpose:
Use the Traffic Shaper
sudo tc qdisc add dev lo root netem delay 10 ms reorder 25% 50% 
