//C IMPORTS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
//Other UNIX IMPORTS
#include <signal.h>
//Socket imports, comment until cygwin is installed
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//Importes de utilidades, comment until makefile is done
//#include "misc.h"
// Constantes: Conexión
#define PUERTO 40040
#define TAM_BUFFER 10
#define MAX_REINTENTOS 2
#define TIEMPO_TIMEOUT 10
// Constantes: Códigos error y salida
#define EXIT_CORRECTO 0
#define EXIT_ERR_GENERICO -1
#define EXIT_ERR_USO -2
#define EXIT_TIMEOUT -3
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
// Funciones
void clienteUDP(ordenes modoOrdenes, char * nombreServidor);
void clienteTCP(ordenes modoOrdenes, char * nombreServidor);
void strToUpper(char * str);
void udpSigAlarmHandler(int signal);
// Variables globales
int reintentos = 0;
boolean timeout;
// Codigo
int main(int argc, char *argv[]) {
  int i;
  protocolo modoProtocolo;
  ordenes modoOrdenes;
  // DEBUG PRINTFS
  printf("argc: %d\n", argc);
  for (i = 0; i < argc; i++) {
      printf("argv[%d]: %s\n", i, argv[i]);
  }
  // Validacion de argumentos
  if (argc == 3 || argc == 4) {
    strToUpper(argv[2]);
    if (!(!strcmp(argv[2], "TCP") || !strcmp(argv[2], "UDP"))) {
      printf("Error de uso. Uso correcto: ./cliente [direccion / hostname] [tcp / udp]");
      exit(EXIT_ERR_USO);
    }
  } else {
    printf("Error de uso. Uso correcto: ./cliente [direccion / hostname] [tcp / udp]");
    exit(EXIT_ERR_USO);
  }
  // Parsing de argumentos
  modoOrdenes = (argc == 4)? MODO_FICHERO : MODO_CONSOLA;
  modoProtocolo = (!strcmp(argv[2], "TCP"))? MODO_TCP : MODO_UDP;
  // Registramos señales para el modo UDP
  if (modoOrdenes == MODO_UDP) {
    //TODO: Bloquear todas las señales menos SIGALARM (UDP) y SIGTERM si hay problemas.
    //TODO: Función de redefinir señales.
    struct sigaction sigAlarmUdp;
    sigAlarmUdp.sa_handler = udpSigAlarmHandler;
    sigAlarmUdp.sa_flags = 0;
    sigfillset(&(sigAlarmUdp.sa_mask));
    if (sigaction(SIGALRM, &sigAlarmUdp, NULL)) {
      fprintf(stderr, "MODO_UDP: Redefinición de señales.");
      exit(EXIT_ERR_GENERICO);
    }
    clienteUDP(modoOrdenes, argv[1]);
  } else {
    clienteTCP(modoOrdenes, argv[1]);
  }
  return 0;
}

void clienteUDP(ordenes modoOrdenes, char * nombreServidor) {
  return;
}

void clienteTCP(ordenes modoOrdenes, char * nombreServidor) {
  struct sockaddr_in servidor;
  int fd_socket;
  char mensajeDebug[32] = "Conexión correcta.";
  fd_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_socket == -1) {
    fprintf(stderr, "clienteTCP: No se pudo crear el socket. nombreServidor: %s", nombreServidor);
    exit(EXIT_ERR_GENERICO);
  }
  // Ponemos a 0 todo el sockaddr_in
  memset(&servidor, 0, sizeof(servidor));
  servidor.sin_family = AF_INET;
  servidor.sin_port = htons(PUERTO);
  inet_pton(AF_INET, nombreServidor, &servidor.sin_addr);
  if (connect(fd_socket, &servidor, sizeof(servidor)) == -1) {
    fprintf(stderr, "clienteTCP: No se pudo conectar.");
    close(fd_socket);
    exit(EXIT_ERR_GENERICO);
  }
  if (modoOrdenes = MODO_CONSOLA) {
    send(fd_socket, mensajeDebug, sizeof(mensajeDebug), 0);
    // while(true) {
    //   break;
    // }
  } else {
    fprintf(stdout, "clienteTCP: MODO_ORDENES no implementado aun");
  }
  close(fd_socket);
  exit(EXIT_CORRECTO);
}

void strToUpper(char * str) {
  int i = 0;
  if (str != NULL) {
    while (str[i] != '\0') {
      str[i] = toupper(str[i]);
      i++;
    }
  } else {
    fprintf(stderr, "strToUpper: String nulo");
  }
}


void udpSigAlarmHandler(int signal) {
  if (reintentos < MAX_REINTENTOS) {
    timeout = TRUE;
    reintentos += 1;
  } else {
    // Salimos
    exit(EXIT_TIMEOUT);
  }
}
