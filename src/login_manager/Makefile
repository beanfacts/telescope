all:
	gcc login_client.c -o login_client -ljson-c
	go build login_server.go
	gcc dumb_client.c -o dumb_client

clean:
	rm login_server
	rm login_client
	rm dumb_client
