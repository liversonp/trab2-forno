# trabalho 2 - Forno Industrial

## Dados do estudante

| Nome                         | Matrícula|
|------------------------------|----------|
| Liverson Paulo Furtado Severo| 180022237|

## Execução e funcionamento do programa

Dentro da pasta do projeto rode o Makefile:

- make

O programa vai inicializar, na tela vai ser possível ver os comandos recebidos via uart.
O forno só vai funcionar quando for ligado para então poder começar a execução.

O aquecimento e resfriamento são feitos utilizando WiringPi / SoftPwm. Comunicando assim com o resistor e também com a ventoinha.

Ao clicar em Ligar, o led vai acender indicando que está ligado. O mesmo ocorre com o iniciar.
De forma inversa, desligar e parar vão desligar os respectivos leds.