multithread_server:multithread_server.c
	gcc -o multithread_server multithread_server.c

socket_client:socket_client.c
	gcc -o socket_client socket_client.c

clean:
	rm -f socket_client multithread_server
