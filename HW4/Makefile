all:
	gcc -std=c11 main.c -lcrypto -o cracker
clean:
	rm ./cracker ./outfile.txt
test:
	./cracker "System Programming" "1234" 4 3 outfile.txt