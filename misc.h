#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
// Constantes: Conexión
#define PUERTO 40040
#define TAM_BUFFER 512
#define MAX_REINTENTOS 2
#define TIEMPO_TIMEOUT 4
// Constantes: Semaforos y sus operaciones.
#define SEMAFORO_LOG 0
#define WAIT -1
#define WAITALL 0
#define SIGNAL 1
// Constantes: Códigos error y salida
#define EXIT_CORRECTO 0
#define EXIT_ERR_GENERICO -1
#define EXIT_ERR_USO -2
#define EXIT_TIMEOUT -3
#define EXIT_ERR_FICHORDENES -4
#define SIN_CODIGO (uint16_t)0
#define CODIGO_PREPARADO (uint16_t)200
#define CODIGO_DESPEDIDA (uint16_t)205
#define CODIGO_GRUPO_SELECCIONADO (uint16_t)211
#define CODIGO_GRUPOS_LISTADO (uint16_t)215
#define CODIGO_ARTICULO_CABECERA_RECUPERADA (uint16_t)221
#define CODIGO_ARTICULO_CUERPO_RECUPERADO (uint16_t)222
#define CODIGO_ARTICULO_RECUPERADO (uint16_t)223
#define CODIGO_ARTICULOS_DIAYHORA (uint16_t)230
#define CODIGO_GRUPOS_DIAYHORA (uint16_t)231
#define CODIGO_ARTICULO_CORRECTO (uint16_t)240
//Este está sacado del PDF.
#define CODIGO_POST_INICIO (uint16_t)340
#define CODIGO_GRUPO_INEXISTENTE (uint16_t)411
#define CODIGO_ARTICULO_INEXISTENTE (uint16_t)423
#define CODIGO_ARTICULO_DESCONOCIDO (uint16_t)430
#define CODIGO_ERROR_SUBIDA (uint16_t)441
#define CODIGO_COMANDO_DESCONOCIDO (uint16_t)500
#define CODIGO_COMANDO_SINTAXIS (uint16_t)501
//Constantes: Rutas
#define RUTA_BASE "nntp/noticias"
#define RUTA_ARTICULOS "nntp/noticias/articulos"
#define RUTA_ARCHIVO_GRUPOS "nntp/noticias/grupos"
#define RUTA_ARCHIVO_NARTICULOS "nntp/noticias/n_articulos"
// Enums
typedef enum {
  FALSE, TRUE
} boolean;
typedef enum {
  MODO_TCP, MODO_UDP
} protocolo;
typedef enum {
  MODO_CONSOLA, MODO_FICHERO
} ordenes;
//TODO: el pack es para mandar estructuras pero no sé si funciona
#pragma pack(1)
typedef struct {
  //Usamos enteros de tamaño fijo para evitar problemas entre maquinas de distintas arquitecturas.
  uint16_t codRespuesta;
  char datos[TAM_BUFFER];
} tipoMensaje;
#pragma pack(0)
// Funciones
void strToUpper(char * str);
char * trim(char * str);
tipoMensaje * constructorCodRespuesta(int codigo);
tipoMensaje * constructorCodYString(int codigo, char * msg, int msgSize, boolean print);
void imprimirMensaje(tipoMensaje * msg);
int operarSobreSemaforo(int semaforo,int indice,short op,short nsops,short flg);
