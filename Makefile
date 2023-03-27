multithreaded:	multithreaded.c
				@gcc multithreaded.c -Wall -Wextra -Wpedantic -Werror -Wno-pointer-arith -pthread -O3 -g -o multithreaded

single:			single.c
				@gcc single.c -Wall -Wextra -Wpedantic -Werror -Wno-pointer-arith -pthread -O3 -g -o single

clean:
	@rm multithreaded single