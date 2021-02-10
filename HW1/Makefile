all: server.c
	gcc server.c -o write_server
	gcc server.c -D READ_SERVER -o read_server
reset:
	@rm -f preorderRecord
	@cp defaultRecord ./preorderRecord
clean:
	rm -f read_server write_server
