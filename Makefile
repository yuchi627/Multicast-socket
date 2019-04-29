client:multicast_client.c
	gcc –o client multicast_client.c 
	
server:multicast_server.c
	gcc –o server multicast_server.c 

clean:
	rm -f client server
