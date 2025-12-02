COMP=gcc
FLAGS= -Wall -fsanitize=address

zad1: zad1.c
	${COMP} ${FLAGS} zad1.c -o zad1
zad2: zad2.c
	${COMP} ${FLAGS} zad2.c -o zad2
zad3: zad3.c
	${COMP} ${FLAGS} zad3.c -o zad3
zad4: zad4.c
	${COMP} ${FLAGS} zad4.c -o zad4
prog1: prog1.c
	${COMP} ${FLAGS} prog1.c -o prog1

prog2: prog2.c
	${COMP} ${FLAGS} prog2.c -o prog2
.PHONY: clean
clean:
	rm -rf zad1 zad2 zad3 zad4 prog1 *.txt