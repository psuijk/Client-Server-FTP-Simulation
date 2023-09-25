all: client server

project1.zip: ftpc.c ftps.c Makefile README
	zip $@ $^

client: ftpc.c
	gcc -o client ftpc.c

server: ftps.c
	gcc -o server ftps.c

clean:
	rm -rf *o client server project1.zip
