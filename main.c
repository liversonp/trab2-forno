#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

void tratasinal(int s){
    printf("Finalizando o programa\n");
    exit(0);
}


int main(){
    signal(SIGINT, tratasinal);
    return 0;
}