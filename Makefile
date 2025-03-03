serverd: server.c
	gcc -o serverd server.c

run: serverd
	./serverd

clean:
	rm -f serverd
