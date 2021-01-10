#TODO: run y clean son especificos de windows
all: cliente servidor run

default: all


run:
	cmd /c start servidor.exe
	cmd /c start cliente.exe localhost TCP 

cliente: cliente.c misc.o misc.h
	gcc cliente.c misc.o misc.h -o cliente

servidor: servidor.c misc.o misc.h
	gcc servidor.c misc.o misc.h -o servidor

misc.o: misc.h
	gcc -c misc.c

.PHONY: clean
clean:
	rm *.o cliente.exe servidor.exe
