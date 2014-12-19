CC=gcc-4.9
EXECUTE=msh
COMPILE_FLAG=
OBJ=obj

all:$(EXECUTE)
	
$(EXECUTE):mini_sh.o re_pi_info.o
	$(CC) $(COMPILE_FLAG) $(OBJ)/mini_sh.o $(OBJ)/re_pi_info.o  -o $(EXECUTE)

mini_sh.o : mini_sh.c re_pi_info.h
	$(CC) -c mini_sh.c -o $(OBJ)/$@

re_pi_info.o : re_pi_info.c re_pi_info.h
	$(CC) -c re_pi_info.c -o $(OBJ)/$@


clear:
	rm -rf $(OBJ)/* $(EXECUTE)
