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
#include "misc.h"
void clienteUDP(ordenes modoOrdenes, char * nombreServidor);
void clienteTCP(ordenes modoOrdenes, char * nombreServidor);
void udpSigAlarmHandler(int signal);
ssize_t recvMensajeEntero(int fd_socket, void * buf, size_t bufsize);
// Variables globales
int reintentos = 0;
boolean timeout;
// Codigo
int main(int argc, char *argv[]) {
  int i;
  protocolo modoProtocolo;
  ordenes modoOrdenes;
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
  printf("-------------------------------------\n");
  printf("CLIENTE NNTP\n");
  printf("MODO DE ENTRADA: %s\n", (modoOrdenes == MODO_FICHERO) ? "ENTRADA DE FICHERO" : "ENTRADA POR CONSOLA");
  printf("PROTOCOLO: %s\n", (modoProtocolo == MODO_TCP) ? "TCP" : "UDP");
  printf("-------------------------------------\n\n");

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
  boolean acabar = FALSE;
  char mensajeDebug[32] = "Conexión correcta.";
  char buffer[512];
  tipoMensaje * mensajeEnviado = malloc(sizeof(tipoMensaje));
  tipoMensaje * mensajeRecibido = malloc(sizeof(tipoMensaje));
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
  if (connect(fd_socket, (const struct sockaddr *) &servidor, sizeof(servidor)) == -1) {
    fprintf(stderr, "clienteTCP: No se pudo conectar.");
    close(fd_socket);
    exit(EXIT_ERR_GENERICO);
  }
  if (modoOrdenes == MODO_CONSOLA) {
    while (acabar != TRUE) {

      fflush(stdin);
      fgets(buffer, TAM_BUFFER, stdin);
      mensajeEnviado = constructorCodYString(SIN_CODIGO, buffer, strlen(buffer), FALSE);
      send(fd_socket, mensajeEnviado, sizeof(tipoMensaje), 0);
      //recvMensajeEntero(fd_socket, mensajeRecibido, sizeof(tipoMensaje));
      recv(fd_socket, mensajeRecibido, sizeof(tipoMensaje), MSG_WAITALL);
      imprimirMensaje(mensajeRecibido);
      switch(mensajeRecibido->codRespuesta) {
        case 205:
          close(fd_socket);
          acabar = TRUE;
          break;
        default:
          break;
      }
    }
    // while(true) {
    //   break;
    // }
  } else {
    fprintf(stdout, "clienteTCP: MODO_ORDENES no implementado aun");
  }
  close(fd_socket);
  exit(EXIT_CORRECTO);
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

ssize_t recvMensajeEntero(int fd_socket, void * buf, size_t bufsize) {
  size_t aLeer = bufsize; // Para bucle para recibir.
  ssize_t yaRecibido;
  while (aLeer > 0) {
    yaRecibido = recv(fd_socket, buf, aLeer, 0);
    if (yaRecibido < 0) {
      perror("recvMensajeEntero: ");
      break;
    }
    aLeer -= yaRecibido; //Leemos menos
    buf += yaRecibido; //Movemos el buffer
  }
  return yaRecibido;
}
