#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef enum {
  FALSE, TRUE
} boolean;
typedef enum {
  MODO_TCP, MODO_UDP
} modoFuncionamiento;
typedef enum {
  MODO_CONSOLA, MODO_FICHERO
} modoOrdenes;
void strToUpper(char * str);
