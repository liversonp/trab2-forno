#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <wiringPi.h>
#include <softPwm.h>
#include <linux/i2c-dev.h>
#include <time.h>

#include "bme280.h"
#include "uart.h"
#include "pid.h"
#include "externa.h"

#define RESISTOR 4
#define VENTOINHA 5

#define I2C_ADDR 0x76

pthread_t t_menu;
pthread_t t_forno;
pthread_t t_temperaturaR;
pthread_t t_armazenamento;

float temperaturaI = 0;
float temperaturaR = 0;
float temperaturaE = 0;
float resPWML = 0;
float ventoinhaPWML = 0;

float kp = 30;
float ki = 0.2;
float kd = 400;

int uartValue;
int ligado = 0;
int funcionamento = 0;

FILE *arquivo;

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
    
    if(access("log.csv", F_OK) == -1){
        arquivo = fopen("log.csv", "w");
        fprintf(arquivo, "Data e Hora, Temp Interna, Temp referencia, Temp externa, Resistor, Ventoinha\n");
        fclose(arquivo);
    }

}

void tratasinal(int s){
    int encerra = 0;
    char buff[4];
    
    memcpy(&buff, (char*)&encerra, sizeof(encerra));
    sendData(uartValue, ENVIA_ESTADO_SIS, buff, 1);

    pthread_cancel(t_menu);
    pthread_cancel(t_forno);
    pthread_cancel(t_temperaturaR);
    pthread_cancel(t_armazenamento);

    softPwmWrite(RESISTOR,0);
    softPwmWrite(VENTOINHA,0);
    
    printf("Finalizando o programa\n");
    exit(0);
}

void medidorTemperaturaInterna(){
    char buffer[40];
    int bufferSize;
    while(1){
        reqData(uartValue, SOLIC_TI);
        usleep(1000000);
        readData(uartValue, buffer, 9);
        memcpy(&temperaturaI, &buffer[3], 4);
        //printf("Temperatura interna: %.2f\%\n", temperaturaI);
    }
}

void *medidorTemperaturaReferencia(){
    char buffer[40];
    int bufferSize;
    while(1){
        reqData(uartValue, SOLIC_TR);
        usleep(1000000);
        readData(uartValue, buffer, 9);
        memcpy(&temperaturaR, &buffer[3], 4);
        //printf("Temperatura referencia: %.2f\%\n", temperaturaR);
    }
}

void medidorTemperaturaExterna(){
    temperaturaE = temperaturaExterna();
}

void *guardaData(){
    while(1){
        usleep(1000000);
        medidorTemperaturaExterna();
        time_t datahora;
        struct tm * timeinfo;
        char tempo_convertido[100];

        time (&datahora);
        timeinfo = localtime(&datahora);
        strcpy(tempo_convertido, asctime(timeinfo));
        tempo_convertido[strlen(tempo_convertido)-1] = '\0';

        arquivo = fopen("log.csv", "a");
        fprintf(arquivo, "%s, %.2f, %.2f, %.2f, %.2f\%, %.2f\%\n", tempo_convertido, temperaturaI, temperaturaR, temperaturaE, resPWML, ventoinhaPWML);
        fclose(arquivo);
    }
}

void esquentarForno(float valorResistor){
    int max = 100;
    char temp[4];

    memcpy(temp,(char*)&max, sizeof(max));
    sendData(uartValue, ENVIA_SINAL_CTRL, temp, 4);

    softPwmWrite(RESISTOR, valorResistor);
    resPWML = valorResistor;
}

void esfriarForno(float valorVentoinha){
    int max = -100;
    char temp[4];

    memcpy(temp, (char*)&max, sizeof(max));
    sendData(uartValue, ENVIA_SINAL_CTRL, temp, 4);

    valorVentoinha *= -1;

    softPwmWrite(VENTOINHA,valorVentoinha);
    ventoinhaPWML = valorVentoinha;
}

void *iniciaPID(){
    int PIDflag = -1;
    int contador = 0;
    int estavel = 0;
    while(1){
        while(funcionamento){
            contador++;
            usleep(1000000);
            PIDflag = pid_controle(temperaturaI);
            if(temperaturaR - temperaturaI <= 0.5 && temperaturaR - temperaturaI >= -0.5 && estavel == 5){
                funcionamento = 0;
            }
            else if(PIDflag > 0){
                estavel = 0;
                esquentarForno(PIDflag);
            }
            else if(PIDflag < 0){
                estavel = 0;
                esfriarForno(PIDflag);
            }
            if(temperaturaI - temperaturaR <= 0.5 && temperaturaI - temperaturaR >= -0.5){
                estavel++;
            }
        }
    }
}

void *menu(){
    int comando;
    char bufferUser[40];
    char buffer[4];

    while(1){
        usleep(500000);
        reqData(uartValue, CMD_USER);
        usleep(1000000);
        readData(uartValue, bufferUser, 9);
        memcpy(&comando, &bufferUser[3], 1);

        printf("Comando do usuario: %d\n", comando);
        switch(comando){
            case 161:
                printf("Comando para ligar o forno\n");
                ligado = 1;
                memcpy(buffer, (char*)&ligado,sizeof(ligado));
                sendData(uartValue, ENVIA_ESTADO_SIS, buffer, 1);
                usleep(1000000);
                readData(uartValue,bufferUser, 9);
                break;
            
            case 162:
                funcionamento = 0;
                ligado = 0;
                printf("Comando para desligar o forno\n");
                memcpy(buffer,(char*)&ligado,sizeof(ligado));
                sendData(uartValue, ENVIA_ESTADO_SIS, buffer, 1);
                usleep(1000000);
                readData(uartValue, bufferUser, 9);

                softPwmWrite(RESISTOR,0);
                softPwmWrite(VENTOINHA,0);
                break;
            
            case 163:
                printf("Comando para iniciar o aquecimento do forno\n");
                if(ligado){
                    printf("Iniciando aquecimento\n");
                    funcionamento = 1;
                    memcpy(buffer, (char*)&funcionamento, sizeof(funcionamento));
                    sendData(uartValue,ENVIA_ESTADO_FUN, buffer,1);
                    usleep(1000000);
                    readData(uartValue, bufferUser, 9);
                }
                break;
            
            case 164:
                printf("Comando para cancelar o processo\n");
                if(ligado){
                    printf("Parando o processo\n");
                    funcionamento = 0;
                    
                    memcpy(buffer, (char*)&funcionamento, sizeof(funcionamento));
                    sendData(uartValue, ENVIA_ESTADO_FUN, buffer, 1);
                    usleep(1000000);
                    readData(uartValue, bufferUser, 9);
                    
                    softPwmWrite(RESISTOR,0);
                    softPwmWrite(VENTOINHA,0);
                }
                break;
            
            case 165:
                printf("Comando para alternar o modo\n");
                break;

        }
    }
}

int main(){

    setupProgram();
    signal(SIGINT, tratasinal);
    pthread_create(&t_menu, NULL, menu, NULL);
    pthread_create(&t_forno, NULL, iniciaPID, NULL);
    pthread_create(&t_temperaturaR,NULL, medidorTemperaturaReferencia, NULL);
    pthread_create(&t_armazenamento,NULL,guardaData,NULL);
    medidorTemperaturaInterna();
    pthread_join(t_menu,NULL);
    pthread_join(t_forno, NULL);
    pthread_join(t_temperaturaR,NULL);
    pthread_join(t_armazenamento,NULL);
    return 0;
}