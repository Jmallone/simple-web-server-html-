
serverd: server.c
	gcc -o serverd server.c
	./serverd
run:
	./serverd

clean:
	rm serverd
