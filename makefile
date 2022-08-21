main: client.o server.o 
	gcc client.o -o client
	gcc server.o -o server

client: client.c
	gcc client.c -c client.o
  	
server: server.c
	gcc server.c -c server.o

clean:
	rm *.o client server
