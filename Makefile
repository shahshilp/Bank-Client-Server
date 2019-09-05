all: bankingClient bankingServer

bankingServer: server.c
	gcc -g -o bankingServer server.c -pthread
bankingClient: client.c
	gcc -g -o bankingClient client.c  -pthread
clean:
	rm -rf bankingServer bankingClient
