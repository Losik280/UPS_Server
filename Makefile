CC=gcc

all:	clean comp

comp:
	${CC} -g server_core.c network_interface.h network_interface.c player_manager.h player_manager.c match_manager.h match_manager.c rules_engine.h rules_engine.c def_n_struct.h -o ups_server -lpthread -lrt -lm -Wall

clean:
	rm -f ups_server
	rm -f *.*~

