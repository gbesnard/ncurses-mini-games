gcc = gcc -std=gnu11 -W

rain_code: pthread_sleep.o rain_code.o
	$(gcc) -o rain_code pthread_sleep.o rain_code.o -lpthread -lcurses

pthread_sleep.o: pthread_sleep.c
	$(gcc) -c pthread_sleep.c -lpthread -lcurses

rain_code.o: rain_code.c pthread_sleep.h
	$(gcc) -c rain_code.c -lpthread -lcurses 

clean:
	rm -rf *.o
