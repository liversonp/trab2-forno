all: build run

build: 
	gcc main.c uart.c bme280.c -o forno -lwiringPi -lpthread
run:    
	./forno "/dev/i2c-1"
clean:
	rm -rf forno