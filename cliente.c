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
void clienteUDP(ordenes modoOrdenes, char * nombreServidor, char * nombreFichOrdenes);
void clienteTCP(ordenes modoOrdenes, char * nombreServidor, char * nombreFichOrdenes);
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
    struct sigaction sigAlarmUdp;
    sigAlarmUdp.sa_handler = udpSigAlarmHandler;
    sigAlarmUdp.sa_flags = SA_RESTART;
    sigfillset(&(sigAlarmUdp.sa_mask));
    if (sigaction(SIGALRM, &sigAlarmUdp, NULL) == -1) {
      fprintf(stderr, "MODO_UDP: Redefinición de señales.");
      exit(EXIT_ERR_GENERICO);
    }
    if (modoOrdenes == MODO_FICHERO) {
      clienteUDP(modoOrdenes, argv[1], argv[3]);
    } else {
      clienteUDP(modoOrdenes, argv[1], NULL);
    }
  } else {
    if (modoOrdenes == MODO_FICHERO) {
      clienteTCP(modoOrdenes, argv[1], argv[3]);
    } else {
      clienteTCP(modoOrdenes, argv[1], NULL);
    }
  }
  return 0;
}

void clienteUDP(ordenes modoOrdenes, char * nombreServidor, char * nombreFichOrdenes) {
  //TODO: Comentar esto
  //TODO: Comprobar que la alarma funciona bien y no hace timeouts de más
  int contadorPost = 0;
  struct sockaddr_in servidor, local;
  boolean acabar = FALSE;
  socklen_t tamSocket = sizeof(struct sockaddr_in);
  ssize_t tam;
  struct hostent * serverInfo;
  int socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
  char buffer[512], bufferPost[512] = {0};
  int tamMensaje;
  FILE * archivoALeer;
  tipoMensaje * mensajeEnviado = malloc(sizeof(tipoMensaje));
  tipoMensaje * mensajeRecibido = malloc(sizeof(tipoMensaje));
  local.sin_family = AF_INET;
  local.sin_addr.s_addr = INADDR_ANY;
  //TODO: Actualmente el puerto local del cliente no es 40040, no sé si es importante. (?) recheck
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
  if (modoOrdenes == MODO_CONSOLA) {
    while (!acabar && reintentos < MAX_REINTENTOS) {
      fflush(stdin);
      alarm(0); //Just in case
      fgets(buffer, TAM_BUFFER, stdin);
      mensajeEnviado = constructorCodYString(SIN_CODIGO, buffer, strlen(buffer), FALSE);
      //Si sendto() falla por la alarma, volverá a empezar por el flag SA_RESTART
      alarm(TIEMPO_TIMEOUT);
      sendto(socketUDP, mensajeEnviado, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, tamSocket);
      // Limpiamos la alarma para evitar que llegue justo cuando empecemos el recvfrom
      alarm(0);
      alarm(TIEMPO_TIMEOUT);
      tamMensaje = recvfrom(socketUDP, mensajeRecibido, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, &tamSocket);
      alarm(0);
      imprimirMensaje(mensajeRecibido);
      // Limpiamos la alarma otra vez para evitar influir en el siguiente comando
      switch(mensajeRecibido->codRespuesta) {
        case CODIGO_DESPEDIDA:
          reintentos = 0;
          acabar = TRUE;
          break;
        case CODIGO_POST_INICIO:
          contadorPost = 0;
          while (!(!(strcmp(trim(buffer), ".")))) {
            fflush(stdin);
            if (contadorPost == 0) printf("Newsgroups: ");
            if (contadorPost == 1) printf("Subject: ");
            fgets(buffer, TAM_BUFFER, stdin);
            sprintf(bufferPost+strlen(bufferPost), "%s\n", buffer);
            if (contadorPost == 1) printf("\n");
            contadorPost++;
          }
          mensajeEnviado = constructorCodYString(SIN_CODIGO, bufferPost, strlen(buffer), FALSE);
          //Si sendto() falla por la alarma, volverá a empezar por el flag SA_RESTART
          alarm(TIEMPO_TIMEOUT);
          sendto(socketUDP, mensajeEnviado, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, tamSocket);
          // Limpiamos la alarma para evitar que llegue justo cuando empecemos el recvfrom
          alarm(0);
          alarm(TIEMPO_TIMEOUT);
          tamMensaje = recvfrom(socketUDP, mensajeRecibido, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, &tamSocket);
          alarm(0);
          imprimirMensaje(mensajeRecibido);
        default:
          break;
      }
    }
  } else {
    if (nombreFichOrdenes == NULL) exit(EXIT_ERR_FICHORDENES);
    archivoALeer = fopen(nombreFichOrdenes, "r");
    fclose(stdin);
    if (archivoALeer == NULL) exit(EXIT_ERR_FICHORDENES);
    while (!acabar && !feof(archivoALeer) && reintentos < MAX_REINTENTOS) {
      fgets(buffer, TAM_BUFFER, archivoALeer);
      mensajeEnviado = constructorCodYString(SIN_CODIGO, buffer, strlen(buffer), FALSE);
      //Si sendto() falla por la alarma, volverá a empezar por el flag SA_RESTART
      alarm(TIEMPO_TIMEOUT);
      sendto(socketUDP, mensajeEnviado, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, tamSocket);
      // Limpiamos la alarma para evitar que llegue justo cuando empecemos el recvfrom
      alarm(0);
      alarm(TIEMPO_TIMEOUT);
      tamMensaje = recvfrom(socketUDP, mensajeRecibido, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, &tamSocket);
      alarm(0);
      imprimirMensaje(mensajeRecibido);
      // Limpiamos la alarma otra vez para evitar influir en el siguiente comando
      switch(mensajeRecibido->codRespuesta) {
        case CODIGO_DESPEDIDA:
          reintentos = 0;
          acabar = TRUE;
          break;
        case CODIGO_POST_INICIO:
          contadorPost = 0;
          break;
        default:
          break;
      }
    }
    pause();
  }
}

void clienteTCP(ordenes modoOrdenes, char * nombreServidor, char * nombreFichOrdenes) {
  struct sockaddr_in servidor;
  int contadorPost = 0;
  int fd_socket;
  boolean acabar = FALSE;
  char mensajeDebug[32] = "Conexión correcta.";
  char buffer[512], bufferPost[512] = {0};
  FILE * archivoALeer;
  tipoMensaje * mensajeEnviado = malloc(sizeof(tipoMensaje));
  tipoMensaje * mensajeRecibido = malloc(sizeof(tipoMensaje));
  fd_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_socket == -1) {
    fprintf(stderr, "clienteTCP: No se pudo crear el socket. nombreServidor: %s", nombreServidor);
    exit(EXIT_ERR_GENERICO);
  }
  // Ponemos a 0 todo el struct de servidor
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
      recv(fd_socket, mensajeRecibido, sizeof(tipoMensaje), 0);
      imprimirMensaje(mensajeRecibido);
      switch(mensajeRecibido->codRespuesta) {
        case CODIGO_DESPEDIDA:
          acabar = TRUE;
          break;
        case CODIGO_POST_INICIO:
          contadorPost = 0;
          while (!(!(strcmp(trim(buffer), ".")))) {
            if (contadorPost == 0) printf("Newsgroups: ");
            if (contadorPost == 1) printf("Subject: ");
            fflush(stdin);
            fgets(buffer, TAM_BUFFER, stdin);
            sprintf(bufferPost+strlen(bufferPost), "%s\n", buffer);
            if (contadorPost == 1) printf("\n");
            contadorPost++;
          }
          mensajeEnviado = constructorCodYString(SIN_CODIGO, bufferPost, strlen(buffer), FALSE);
          send(fd_socket, mensajeEnviado, sizeof(tipoMensaje), 0);
          recv(fd_socket, mensajeRecibido, sizeof(tipoMensaje), 0);
          imprimirMensaje(mensajeRecibido);
        default:
          break;
      }
    }
    // while(true) {
    //   break;
    // }
  } else {
    if (nombreFichOrdenes == NULL) exit(EXIT_ERR_FICHORDENES);
    archivoALeer = fopen(nombreFichOrdenes, "r");
    fclose(stdin);
    while (!feof(archivoALeer) && acabar != TRUE) {
      fgets(buffer, TAM_BUFFER, archivoALeer);
      mensajeEnviado = constructorCodYString(SIN_CODIGO, buffer, strlen(buffer), FALSE);
      send(fd_socket, mensajeEnviado, sizeof(tipoMensaje), 0);
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
    pause(); //Para que no se cierre automaticamente
  }
  close(fd_socket);
  exit(EXIT_CORRECTO);
}


void udpSigAlarmHandler(int signal) {
  if (reintentos < MAX_REINTENTOS) {
    reintentos++;
  } else {
    // TODO: Cleanup
    timeout = TRUE;
    exit(EXIT_TIMEOUT);
  }
}
