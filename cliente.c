//C IMPORTS
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <errno.h>
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
  if (modoProtocolo == MODO_UDP) {
    //TODO: Bloquear todas las señales menos SIGALARM (UDP) y SIGTERM si hay problemas.
    //TODO: Función de redefinir señales.
    struct sigaction sigAlarmUdp;
    sigAlarmUdp.sa_handler = udpSigAlarmHandler;
    sigAlarmUdp.sa_flags = SA_RESTART;
    sigfillset(&(sigAlarmUdp.sa_mask));
    if (sigaction(SIGALRM, &sigAlarmUdp, NULL) == -1) {
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
  //TODO: Redefinir la alarma para evitar bloqueos en recvfrom en esto y servidor.
  //TODO: Comentar esto
  struct sockaddr_in servidor, local;
  boolean acabar = FALSE;
  socklen_t tamSocket = sizeof(struct sockaddr_in);
  ssize_t tam;
  struct hostent * serverInfo;
  int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
  char buffer[512];
  int tamMensaje;
  tipoMensaje * mensajeEnviado = malloc(sizeof(tipoMensaje));
  tipoMensaje * mensajeRecibido = malloc(sizeof(tipoMensaje));
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = INADDR_ANY;
  //TODO: Actualmente el puerto local del cliente no es 40040, no sé si es importante.
  local.sin_port = 0;
  servidor.sin_family = AF_INET;
  servidor.sin_port = htons(PUERTO);
  inet_pton(AF_INET, nombreServidor, &servidor.sin_addr);
  if (socketUDP == -1) {
    fprintf(stderr, "clienteUDP: No se pudo crear el socket. nombreServidor: %s", nombreServidor);
    exit(EXIT_ERR_GENERICO);
  }
  if ((bind(socketUDP, (const struct sockaddr *)&local, tamSocket)) == -1) {
    fprintf(stderr, "clienteUDP: bind()");
    exit(EXIT_ERR_GENERICO);
  }
  // if ((getsockname(socketUDP, (struct sockaddr *)&local, &tamSocket)) == -1) {
  //   fprintf(stderr, "clienteUDP: getsockname()");
  //   exit(EXIT_ERR_GENERICO);
  // }
  if ((serverInfo = gethostbyname(nombreServidor)) == NULL) {
    fprintf(stderr, "clienteUDP: gethostbyname()");
    exit(EXIT_ERR_GENERICO);
  }
  memcpy((void *)&servidor.sin_addr, serverInfo->h_addr_list[0], serverInfo->h_length);
  while (!acabar && reintentos < MAX_REINTENTOS) {
    //TODO: Implementar alarm para proccear eintr si el servidor no responde en TIMEOUT segs
    //MUY IMPORTANTE!!!
    fflush(stdin);
    fgets(buffer, TAM_BUFFER, stdin);
    mensajeEnviado = constructorCodYString(SIN_CODIGO, buffer, strlen(buffer), FALSE);
    // Entramos en el bucle una vez, y si se nos interrumpe por una señal (la alarma del timeout) volvemos a intentar mandar / enviar
    alarm(TIEMPO_TIMEOUT);
    sendto(socketUDP, mensajeEnviado, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, tamSocket);
    //recvMensajeEntero(fd_socket, mensajeRecibido, sizeof(tipoMensaje));
    // Limpiamos la alarma para evitar que llegue justo cuando empecemos el recvfrom
    // alarm(0);
    alarm(TIEMPO_TIMEOUT);
    tamMensaje = recvfrom(socketUDP, mensajeRecibido, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, &tamSocket);
    imprimirMensaje(mensajeRecibido);
    // Limpiamos la alarma otra vez para evitar influir en el siguiente comando
    // alarm(0);
    switch(mensajeRecibido->codRespuesta) {
      case 205:
        reintentos = 0;
        acabar = TRUE;
        break;
      default:
        break;
    }
  }
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
  //TODO: Cambiar memset a calloc (y usamos punteros directamente)
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
      send(fd_socket, mensajeEnviado, sizeof(mensajeEnviado), 0);
      //recvMensajeEntero(fd_socket, mensajeRecibido, sizeof(tipoMensaje));
      recv(fd_socket, mensajeRecibido, sizeof(tipoMensaje), MSG_WAITALL);
      imprimirMensaje(mensajeRecibido);
      switch(mensajeRecibido->codRespuesta) {
        case 205:
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
    printf("Nah de locos la alarma");
    reintentos++;
  } else {
    // TODO: Cleanup
    timeout = TRUE;
    exit(EXIT_TIMEOUT);
  }
}
