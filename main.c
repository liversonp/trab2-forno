#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>
#include <wiringPi.h>

#include "bme280.h"
#include "uart.h"
#include "pid.h"

#define RESISTOR 4
#define VENTOINHA 5

#define I2C_ADDR 0x76

pthread_t t_menu;


float temperaturaI = 0;
float temperaturaR = 0;
float temperaturaE = 0;

float kp = 30;
float ki = 0.2;
float kd = 400;

int uartValue;
int ligado = 0;

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
    if(wiringPiSetup() == -1){
        exit(1);
    }

    pinMode(RESISTOR, OUTPUT);
    pinMode(VENTOINHA, OUTPUT);

    softPwmCreate(RESISTOR,0,100);
    softPwmCreate(VENTOINHA,0,100);

}

void tratasinal(int s){
    printf("Finalizando o programa\n");
    pthread_cancel(t_menu);
    exit(0);
}

void medidorTemperaturaInterna(){
    char buffer[40];
    int bufferSize;
    int bme;
    uint32_t delay;
    while(1){
        reqData(uartValue, SOLIC_TI);
        usleep(1000000);
        readData(uartValue, buffer, 9);
        memcpy(&temperaturaI, &buffer[3], 4);
        printf("Temperatura interna: %.2f\%\n", temperaturaI);
    }
}

void *menu(){
    int comando;
    char buffer[40];

    while(1){
        usleep(500000);
        reqData(uartValue, CMD_USER);
        usleep(1000000);
        readData(uartValue, buffer, 9);
        memcpy(&comando, &buffer[3], 1);

        printf("Comando do usuario: %d\n", comando);
        switch(comando){
            case 161:
                printf("Comando para ligar o forno\n");
                ligado = 1;
                memcpy(buffer, (char*)&ligado,sizeof(ligado));
                sendData(uartValue, ENVIA_ESTADO_SIS, buffer, 1);
                usleep(1000000);
                readData(uartValue,buffer, 9);
                break;
            
            case 162:
                printf("Comando para desligar o forno\n");
                ligado = 0;
                memcpy(buffer,(char*)&ligado,sizeof(ligado));
                sendData(uartValue, ENVIA_ESTADO_SIS, buffer, 1);
                usleep(1000000);
                readData(uartValue, buffer, 9);
                break;
            
            case 163:
                printf("Comando para iniciar o aquecimento do forno\n");
                if(ligado){
                    printf("Iniciando aquecimento\n");
                }
                break;
            
            case 164:
                printf("Comando para cancelar o processo\n");
                if(ligado){
                    printf("Parando o processo\n");
                }
                break;
            
            case 165:
                printf("Comando para alternar o modo\n");
                break;

        }
    }
}

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("Eh necessario passar parametros para o funcionamento do programa!!\n");
        exit(1);
    }

    setupProgram();
    signal(SIGINT, tratasinal);
    pthread_create(&t_menu, NULL, menu, NULL);
    medidorTemperaturaInterna();
    pthread_join(t_menu,NULL);
    return 0;
}