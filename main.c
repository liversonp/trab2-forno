#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>

#include "bme280.h"
#include "uart.h"

float temperaturaI = 0;
float temperaturaR = 0;
float temperaturaE = 0;

int uartValue;

int setupUart(){
    int filestream = -1;

    filestream = open("/dev/serial0", O_RDWR | O_NOCTTY | O_NDELAY);
    
    if (filestream == -1){
        printf("Erro - Não foi possível iniciar a UART.\n");
    }
    else{
        printf("UART inicializada!\n");
    }

    return filestream;
};

void setupProgram(){
    uartValue = setupUart();
}

void tratasinal(int s){
    printf("Finalizando o programa\n");
    exit(0);
}

void medidorTemperatura(){
    char buffer[40];
    int bufferSize;
    int bme;
    uint32_t delay;
    while(1){
        reqData(uartValue, SOLIC_TI);
        usleep(1000000);
        readData(uartValue, buffer, 9);
        memcpy(&temperaturaI, &buffer[3], 4);

        printf("Temperatura interna: %.2f\n", temperaturaI);
    }
}

int main(){
    signal(SIGINT, tratasinal);
    setupProgram();
    medidorTemperatura();
    return 0;
}