CCFLAGS  = -lm

all: profile
clean:
	rm -f *.o profile
	
profile:
	gcc profile.c -pthread -O0 -I./self_compiled_openssl/include/ -L./self_compiled_openssl/ -lcrypto -o profile

%.o : %.c 
	gcc -c $(CCFLAGS) $<
