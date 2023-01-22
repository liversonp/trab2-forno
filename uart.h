#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>

#define SOLIC_TI 0xC1
#define SOLIC_TR 0xC2
#define CMD_USER 0xC3

#define ENVIA_SINAL_CTRL 0xD1
#define ENVIA_SINAL_REF 0xD2
#define ENVIA_ESTADO_SIS 0xD3
#define MODO_CONTROLE 0xD4
#define ENVIA_ESTADO_FUN 0xD5
#define ENVIA_TA 0xD6

short CRC16(short crc, char data);

short calcula_CRC(unsigned char *commands, int size);

int printBuffer(unsigned char *buf, int size);

int reqData(int fileStream, int dataType);

int sendData(int fileStream, int dataType, char *data, int size);

int readData(int fileStream, char *bufferRetorno, int size);