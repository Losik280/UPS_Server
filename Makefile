CC=gcc

all:	clean comp

comp:
	${CC} -g main.c communication.h communication.c client_manager.h client_manager.c game_manager.h game_manager.c game_logic.h game_logic.c -o ups_server -lpthread -lrt -lm -Wall

clean:
	rm -f ups_server
	rm -f *.*~

