#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>



#define PUERTO 40040
#define TAM_BUFFER 512
#define BACKLOG 5
#define EXIT_CORRECTO 0
#define EXIT_ERR_GENERICO -1
#define EXIT_ERR_USO -2
#define EXIT_TIMEOUT -3
extern int errno;

int main(int argc, char * argv) {
  //TODO: Implementación de UDP también
  // socketListen = ls_tcp, socketTCP = s_tcp en las practicas.
  int socketListenTCP, socketTCP, socketUDP;

  int tamMensaje;
  char bufferMensaje[TAM_BUFFER];
  struct sockaddr_in servidor, cliente;
  socklen_t tamSocket = sizeof(struct sockaddr_in);
  servidor.sin_family = AF_INET;
  servidor.sin_addr.s_addr = INADDR_ANY;
  servidor.sin_port = htons(PUERTO);
  // Creamos los sockets
  socketListenTCP = socket(AF_INET, SOCK_STREAM, 0);
  if (socketListenTCP == -1) {
    fprintf(stderr, "socketTCP: Error de creación de socket.");
  }
  // Bind al socket TCP
  if (bind(socketListenTCP, (const struct sockaddr *)&servidor, sizeof(servidor)) == -1) {
    fprintf(stderr, "socketListenTCP: Error de bind.");
    close(socketListenTCP);
    exit(EXIT_ERR_GENERICO);
  }
  // Listen al socket TCP
  if (listen(socketListenTCP, BACKLOG) == -1) {
    fprintf(stderr, "socketListenTCP: Error de listen.");
    close(socketListenTCP);
    exit(EXIT_ERR_GENERICO);
  }
  // Accept al socket TCP
  if ((socketTCP = accept(socketListenTCP, (struct sockaddr *)&cliente, &tamSocket)) == -1) {
    fprintf(stderr, "socketTCP: Error de accept.");
    close(socketListenTCP);
    close(socketTCP);
    exit(EXIT_ERR_GENERICO);
  }
  //Recibimos el mensaje
  tamMensaje = recv(socketTCP, bufferMensaje, TAM_BUFFER, 0);
  if (tamMensaje <= 0) {
    //TODO: Tratamiento de errores.
    fprintf(stderr, "socketTCP: Error de recv.");
  }
  printf("Mensaje: %s", bufferMensaje);

  return 0;
}
