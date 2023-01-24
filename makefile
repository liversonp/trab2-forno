all: build run

build: 
	gcc main.c uart.c bme280.c pid.c externa.c -o forno -lwiringPi -lpthread
run:    
	./forno
clean:
	rm -rf forno