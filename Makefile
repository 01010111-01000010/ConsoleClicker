all:
	@gcc -o clicker main.c -lcurses -pthread

test:
	@gcc -o clicker test.c -lcurses -pthread