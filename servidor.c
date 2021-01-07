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
#include <dirent.h>
#include "misc.h"
#define IMPRIMIR_MENSAJES TRUE
#define BACKLOG 5
extern int errno;
boolean acabar;
char * rutaGrupoSeleccionado = NULL;
tipoMensaje * procesarComando(tipoMensaje * mensajeEntrada);
void comandoList(char * arg, char * buf, int * cod);
void comandoGroup(char * arg, char * buf, int * cod);
void comandoNewNews(char * arg, char * buf, int * cod);
void comandoNewGroups(char * arg, char * buf, int * cod);
void comandoArticle(char * arg, char * buf, int * cod);
void comandoHead(char * arg, char * buf, int * cod);
void comandoBody(char * arg, char * buf, int * cod);
void comandoPost(char * arg, char * buf, int * cod);
int main(int argc, char * argv) {
  // socketListen = ls_tcp, socketTCP = s_tcp en las practicas.
  struct linger linger;
  linger.l_onoff = 1;
  linger.l_linger = 1;
  int socketListenTCP, socketTCP, socketUDP, socketMayor, nElegido;
  int tamMensaje;
  tipoMensaje * mensajeRecibido = malloc(sizeof(tipoMensaje));
  tipoMensaje * mensajeRespuesta;
  char host[96];
  struct sockaddr_in servidor, cliente, clienteUDP;
  socklen_t tamSocket = sizeof(struct sockaddr_in);
  servidor.sin_family = AF_INET;
  servidor.sin_addr.s_addr = INADDR_ANY;
  servidor.sin_port = htons(PUERTO);
  // Creamos los sockets
  socketListenTCP = socket(AF_INET, SOCK_STREAM, 0);
  socketUDP = socket(AF_INET, SOCK_DGRAM, 0);
  if (socketListenTCP == -1) {
    fprintf(stderr, "socketTCP: Error de creación de socket.");
  }
  // Bind al socket TCP
  if (bind(socketListenTCP, (const struct sockaddr *)&servidor, sizeof(servidor)) == -1) {
    fprintf(stderr, "socketListenTCP: Error de bind.");
    close(socketListenTCP);
    exit(EXIT_ERR_GENERICO);
  }
  // Bind al socket UDP
  if (bind(socketUDP, (const struct sockaddr *)&servidor, sizeof(servidor)) == -1) {
    fprintf(stderr, "socketUDP: Error de bind.");
    close(socketUDP);
    exit(EXIT_ERR_GENERICO);
  }
  // Listen al socket TCP
  if (listen(socketListenTCP, BACKLOG) == -1) {
    fprintf(stderr, "socketListenTCP: Error de listen.");
    close(socketListenTCP);
    exit(EXIT_ERR_GENERICO);
  }
  printf("-------------------------------------\n");
  printf("SERVIDOR NNTP\n");
  printf("-------------------------------------\n\n");
  //Recibimos el mensaje
  //TODO: Bucle que acepte conexiones en distintos protocolos y haga hijos
  //TODO: Poner al servidor como daemon
  while (acabar != TRUE) {
    fd_set socketSet;
    pid_t serverPID;
    socketMayor = (socketUDP > socketListenTCP)? socketUDP : socketListenTCP;
    FD_ZERO(&socketSet);
    FD_SET(socketListenTCP, &socketSet);
    FD_SET(socketUDP, &socketSet);
    nElegido = select(socketMayor+1, &socketSet, (fd_set *)0, (fd_set *)0, NULL);
    if (FD_ISSET(socketListenTCP, &socketSet)) {
      // Accept al socket TCP
      // TODO: Implementación de esto con threads / procesos.
      if ((socketTCP = accept(socketListenTCP, (struct sockaddr *)&cliente, &tamSocket)) == -1) {
        fprintf(stderr, "socketTCP: Error de accept.");
        close(socketUDP);
        close(socketListenTCP);
        close(socketTCP);
        exit(EXIT_ERR_GENERICO);
      }
      getnameinfo((struct sockaddr *)&cliente, tamSocket, host, 0, NULL, 0, 0);
      printf("Conectado a %s por TCP.\n", host);
      while (acabar != TRUE) {
        // serverPID = fork();
        // switch(serverPID) {
        //   case -1:
        //     fprintf(stderr, "fork: Error al crear hijo.");
        //     close(socketUDP);
        //     close(socketListenTCP);
        //     close(socketTCP);
        //     exit(EXIT_ERR_GENERICO);
        //     case 0:
        // close(socketListenTCP);
        tamMensaje = recv(socketTCP, mensajeRecibido, sizeof(tipoMensaje), 0);
        if (tamMensaje <= 0) {
          fprintf(stderr, "socketTCP: Error de recv.");
          close(socketListenTCP);
          close(socketTCP);
          exit(EXIT_ERR_GENERICO);
        }
        mensajeRespuesta = procesarComando(mensajeRecibido);
        if (send(socketTCP, mensajeRespuesta, sizeof(tipoMensaje), 0) < 0) {
          perror("servidorTCP: send: ");
        }
        free(mensajeRespuesta);
        //exit(EXIT_CORRECTO);
        //     default:
        //       break;
        //
        // }
      }
    }
    if (FD_ISSET(socketUDP, &socketSet)) {
      getnameinfo((struct sockaddr *)&servidor, tamSocket, host, 0, NULL, 0, 0);
      printf("Conectado a %s por TCP.\n", host);
      while (acabar != TRUE) {
        tamMensaje = recvfrom(socketUDP, mensajeRecibido, TAM_BUFFER, 0, (struct sockaddr *)&servidor, &tamSocket);
        if (tamMensaje <= 0){
          fprintf(stderr, "socketTCP: Error de recv.");
          close(socketListenTCP);
          close(socketTCP);
          exit(EXIT_ERR_GENERICO);
        }
        mensajeRespuesta = procesarComando(mensajeRecibido);
        if (sendto(socketUDP, mensajeRespuesta, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, tamSocket) < 0) {
          perror("servidorUDP: sendto");
        }
        free(mensajeRespuesta);
      }
    }
  }
  close(socketListenTCP);
  close(socketTCP);

  return EXIT_CORRECTO;
}

/* ------------------------------------------------------------------------------------- */
tipoMensaje * procesarComando(tipoMensaje * mensajeEntrada) {
  //TODO: Implementar trimming en TODOS los comandos. Algunos crashean poniendo espacios como ultimo carácter...
  //TOOD: Funcion que haga strtok_r varias veces
  //TODO: Implementar ERRNO personalizado para las cosas que no tienen codigo correspondiente (archivo no encontrado, etc)
  tipoMensaje * msg;
  int contador = 0;
  boolean sintaxisIncorrecta = FALSE, encontrado = FALSE;
  uint16_t codMensaje = SIN_CODIGO;
  char bufferMensaje[TAM_BUFFER];
  char bufferRespuesta[TAM_BUFFER] = {0}; //Iniciamos todo el buffer a \0
  char argumentos[256] = {0}, bufferLinea[256], grupo[32] = {0}, fecha[32] = {0}, rutaCarpeta[64] = {0}, rutaArchivo[80] = {0};
  char * comando, *saveptr, *saveptr2, *tmp;
  DIR * carpeta;
  FILE * archivoALeer;
  struct dirent * entry, * ultimaEntry, * firstEntry;
  size_t tamArchivo;
  memcpy(bufferMensaje, mensajeEntrada->datos, TAM_BUFFER);
  comando = strtok_r(bufferMensaje, " ", &saveptr);
  strToUpper(comando);
  if (strlen(comando) == 0) comando = bufferMensaje;
  comando = trim(comando);
  //Me gustaría hacerlo con un switch pero es imposible en C.
  if (strcmp(comando, "LIST") == 0) {
    //
    // COMIENZO DE LIST
    //
    archivoALeer = fopen(RUTA_ARCHIVO_GRUPOS, "r");
    if (archivoALeer != NULL) {
      fseek(archivoALeer, 0, SEEK_END);
      tamArchivo = ftell(archivoALeer);
      rewind(archivoALeer);
      fread(bufferRespuesta, 1, tamArchivo, archivoALeer);
      codMensaje = CODIGO_GRUPOS_LISTADO;
      fclose(archivoALeer);
    } else {
      sprintf(bufferRespuesta, "%s: Archivo %s no encontrado.", comando, RUTA_ARCHIVO_GRUPOS);
      codMensaje = CODIGO_GRUPO_INEXISTENTE;
    }
    //
    // FINAL DE LIST
    //
  } else if (strcmp(comando, "NEWGROUPS") == 0) {
    //
    // COMIENZO DE NEWGROUPS
    //
    contador = 0;
    while ((comando = strtok_r(NULL, " ", &saveptr)) != NULL) {
      sprintf(argumentos+strlen(argumentos), "%s", trim(comando));
      if (contador == 0) sprintf(argumentos+strlen(argumentos), " ");
      contador++;
    }
    if (contador >= 2) {
      archivoALeer = fopen(RUTA_ARCHIVO_GRUPOS, "r");
      if (archivoALeer != NULL) {
        while (!feof(archivoALeer)) {
          if (fgets(bufferLinea, sizeof(bufferLinea), archivoALeer) <= 0) break;
          tmp = strtok_r(bufferLinea, " ", &saveptr2);
          sprintf(grupo, "%s", tmp);
          // Quemamos dos tokens (son el número de posts)
          tmp = strtok_r(NULL, " ", &saveptr2);
          tmp = strtok_r(NULL, " ", &saveptr2);
          // Los siguientes dos tokens son la fecha
          tmp = strtok_r(NULL, " ", &saveptr2);
          sprintf(fecha, "%s", tmp);
          tmp = strtok_r(NULL, " ", &saveptr2);
          sprintf(fecha+strlen(fecha), " %s", tmp);
          if (strcmp(argumentos, fecha) < 0) {
            sprintf(bufferRespuesta+strlen(bufferRespuesta), "%s\n", grupo);
            codMensaje = CODIGO_GRUPOS_DIAYHORA;
            encontrado = TRUE;
          }
          saveptr = NULL; //Reseteamos el puntero para mayor uso en strtok_r
        }
        fclose(archivoALeer);
      } else {
        sprintf(bufferRespuesta, "Archivo no encontrado.");
        codMensaje = CODIGO_GRUPO_INEXISTENTE;
      }
      if (!encontrado && codMensaje == SIN_CODIGO) {
        sprintf(bufferRespuesta, "No hay grupos nuevos desde el %s", argumentos);
        codMensaje = CODIGO_GRUPO_INEXISTENTE;
      }
    } else {
      sprintf(bufferRespuesta, "Error de sintaxis: no se encontraron suficientes tokens.");
      codMensaje = CODIGO_COMANDO_SINTAXIS;
    }
    //
    // FIN DE NEWGROUPS
    //
  } else if (strcmp(comando, "NEWNEWS") == 0) {
    //
    // COMIENZO DE NEWNEWS
    //
    while ((comando = strtok_r(NULL, " ", &saveptr)) != NULL) {
      if (contador < 2) sprintf(grupo, "/%s", trim(comando));
      else if (contador < 4){
        sprintf(argumentos+strlen(argumentos), "%s", trim(comando));
        if (contador == 2) sprintf(argumentos+strlen(argumentos), " ");
      }
      contador++;
    }
    sprintf(rutaArchivo, "%s%s", RUTA_ARCHIVO_GRUPOS, grupo);
    if (contador >= 3) {
      carpeta = opendir(rutaArchivo);
      if (carpeta != NULL) {
        sprintf(bufferRespuesta, "Articulos nuevos en %s desde %s: ", grupo, argumentos);
        while ((entry = readdir(carpeta)) != NULL) {
          //TODO: Esto no consigue el primero :/
          if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            sprintf(rutaArchivo, "%s/%s", rutaCarpeta, entry->d_name);
            archivoALeer = fopen(rutaArchivo, "r");
            if (archivoALeer != NULL) {
              for (contador = 0; contador < 3 && !feof(archivoALeer); contador++) {
                fgets(bufferLinea, sizeof(bufferLinea), archivoALeer);
              }
              if (!feof(archivoALeer)) {
                //Quemamos un token
                tmp = strtok_r(bufferLinea, " ", &saveptr2);
                // Los siguientes dos tokens son la fecha
                tmp = strtok_r(NULL, " ", &saveptr2);
                sprintf(fecha, "%s", tmp);
                tmp = strtok_r(NULL, " ", &saveptr2);
                sprintf(fecha+strlen(fecha), " %s", tmp);
                if (strcmp(argumentos, fecha) < 0) {
                  sprintf(bufferRespuesta+strlen(bufferRespuesta), "%s\n", grupo);
                  codMensaje = CODIGO_ARTICULOS_DIAYHORA;
                  encontrado = TRUE;
                }
              }
              fclose(archivoALeer);
            }
          }
          saveptr = NULL; //Reseteamos el puntero para mayor uso en strtok_r
        }
        closedir(carpeta);
      } else {
        sprintf(bufferRespuesta, "Archivo %s no encontrado.", rutaArchivo);
        codMensaje = CODIGO_ARTICULO_INEXISTENTE;
      }
      if (!encontrado && codMensaje == SIN_CODIGO) {
        sprintf(bufferRespuesta, "No hay articulos nuevos desde el %s", argumentos);
        codMensaje = CODIGO_ARTICULO_INEXISTENTE;
      }
    } else {
      sprintf(bufferRespuesta, "Error de sintaxis: no se encontraron suficientes tokens.");
      codMensaje = CODIGO_COMANDO_SINTAXIS;
    }
    //
    // FIN DE NEWNEWS
    //
  } else if (strcmp(comando, "GROUP") == 0) {
    //
    // COMIENZO DE GROUP
    //
    sprintf(argumentos, "%s", RUTA_ARTICULOS);
    while (comando = strtok_r(NULL, ".", &saveptr)) {
      //se puede simplificar y añadir la logica al while pero bueno
      if (comando == NULL) {
        if (contador == 0) {
          sintaxisIncorrecta = TRUE;
        }
        break;
      }
      contador++;
      sprintf(argumentos+strlen(argumentos), "/%s", comando);
    }
    if (!sintaxisIncorrecta) {
      trim(argumentos);
      carpeta = opendir(argumentos);
      if (carpeta == NULL) {
        sprintf(bufferRespuesta, "Grupo %s no encontrado.\n", argumentos);
        codMensaje = CODIGO_GRUPO_INEXISTENTE;
      } else {
        rutaGrupoSeleccionado = malloc(strlen(argumentos) + 1);
        strcpy(rutaGrupoSeleccionado, argumentos);
        contador = 0;
        while ((entry = readdir(carpeta)) != NULL) {
          //TODO: Esto no consigue el primer mensaje, quizas cogerlo de grupos? :/
          //TODO: ESTO NO CHEQUEA SI LOS ARCHIVOS SON DIRECTORIOS, por tanto cosas como "group local" son validas a pesar de que en vez de articulos habrá carpetas dentro.
          if (contador == 3) firstEntry = entry; //3 porque readdir siempre leera primero la carpeta . y ..
          contador++;
          ultimaEntry = entry;
        }
        sprintf(bufferRespuesta, "%d %s %s", contador, firstEntry->d_name, ultimaEntry->d_name);
        codMensaje = CODIGO_ARTICULOS_SELECCIONADOS;
      }
      closedir(carpeta);
    } else {
      sprintf(bufferRespuesta, "Error de sintaxis: no se encontraron suficientes tokens.");
      codMensaje = CODIGO_COMANDO_SINTAXIS;
    }
    //
    // FIN DE GROUP
    //
  } else if (strcmp(comando, "ARTICLE") == 0) {
    //
    // COMIENZO DE ARTICLE
    //
    if ((comando = strtok_r(NULL, "", &saveptr)) == NULL) {
      sprintf(bufferRespuesta, "Error de sintaxis: no se encontraron suficientes tokens.");
      codMensaje = CODIGO_COMANDO_SINTAXIS;
    } else {
      if ((rutaGrupoSeleccionado == NULL)) {
        sprintf(bufferRespuesta, "Error: no se encontró el artículo. ¿Recordaste elegir grupo antes de pedir un archivo?");
        codMensaje = CODIGO_ARTICULO_DESCONOCIDO;
      } else {
        sprintf(argumentos, "%s/%s", rutaGrupoSeleccionado, trim(comando));
        archivoALeer = fopen(argumentos, "r");
        if (archivoALeer != NULL) {
          fseek(archivoALeer, 0, SEEK_END);
          tamArchivo = ftell(archivoALeer);
          rewind(archivoALeer);
          fread(bufferRespuesta, 1, tamArchivo, archivoALeer);
          codMensaje = CODIGO_ARTICULO_RECUPERADO;
          fclose(archivoALeer);
        } else {
          sprintf(bufferRespuesta, "%s: Archivo %s no encontrado.", comando, RUTA_ARCHIVO_GRUPOS);
          codMensaje = CODIGO_ARTICULO_INEXISTENTE;
        }
      }
    }
    //
    // FIN DE ARTICLE
    //
  } else if (strcmp(comando, "HEAD") == 0) {
    //
    // COMIENZO DE HEAD
    //
    if ((comando = strtok_r(NULL, "", &saveptr)) == NULL) {
      sprintf(bufferRespuesta, "Error de sintaxis: no se encontraron suficientes tokens.");
      codMensaje = CODIGO_COMANDO_SINTAXIS;
    } else {
      if ((rutaGrupoSeleccionado == NULL)) {
        sprintf(bufferRespuesta, "Error: no se encontró el artículo. ¿Recordaste elegir grupo antes de pedir un archivo?");
        codMensaje = CODIGO_ARTICULO_DESCONOCIDO;
      } else {
        sprintf(argumentos, "%s/%s", rutaGrupoSeleccionado, trim(comando));
        archivoALeer = fopen(argumentos, "r");
        if (archivoALeer != NULL) {
          for (contador = 0; contador < 5 && !feof(archivoALeer); contador++) {
            //TODO: Error checking aqui
            fgets(bufferLinea, sizeof(bufferLinea), archivoALeer);
            sprintf(bufferRespuesta+strlen(bufferRespuesta), "%s", bufferLinea);
          }
          codMensaje = CODIGO_ARTICULO_CABECERA_RECUPERADA;
          fclose(archivoALeer);
        } else {
          sprintf(bufferRespuesta, "%s: Archivo %s no encontrado.", comando, RUTA_ARCHIVO_GRUPOS);
          codMensaje = CODIGO_ARTICULO_INEXISTENTE;
        }
      }
    }
    //
    // FIN DE HEAD
    //
  } else if (strcmp(comando, "BODY") == 0) {
    //
    // COMIENZO DE BODY
    //
    if ((comando = strtok_r(NULL, "", &saveptr)) == NULL) {
      sprintf(bufferRespuesta, "Error de sintaxis: no se encontraron suficientes tokens.");
      codMensaje = CODIGO_COMANDO_SINTAXIS;
    } else {
      if ((rutaGrupoSeleccionado == NULL)) {
        sprintf(bufferRespuesta, "Error: no se encontró el artículo. ¿Recordaste elegir grupo antes de pedir un archivo?");
        codMensaje = CODIGO_ARTICULO_DESCONOCIDO;
      } else {
        sprintf(argumentos, "%s/%s", rutaGrupoSeleccionado, trim(comando));
        archivoALeer = fopen(argumentos, "r");
        if (archivoALeer != NULL) {
          for (contador = 0; !feof(archivoALeer); contador++) {
            fgets(bufferLinea, sizeof(bufferLinea), archivoALeer);
            if (contador >= 5) {
              //TODO: Error checking aqui
              sprintf(bufferRespuesta+strlen(bufferRespuesta), "%s", bufferLinea);
            }
          }
          codMensaje = CODIGO_ARTICULO_CUERPO_RECUPERADO;
          fclose(archivoALeer);
        } else {
          sprintf(bufferRespuesta, "%s: Archivo %s no encontrado.", comando, RUTA_ARCHIVO_GRUPOS);
          codMensaje = CODIGO_ARTICULO_INEXISTENTE;
        }
      }
    }
    //
    // FIN DE BODY
    //
  } else if (strcmp(comando, "POST") == 0) {
    //
    // COMIENZO DE POST
    //
    // TODO: POST
    sprintf(bufferRespuesta, "%s: NO IMPLEMENTADO", comando);
    codMensaje = CODIGO_COMANDO_DESCONOCIDO;
    //
    // FIN DE POST
    //
  } else if (strcmp(comando, "QUIT") == 0) {
    //
    // COMIENZO DE QUIT
    //
    sprintf(bufferRespuesta, "Despedida.");
    codMensaje = CODIGO_DESPEDIDA;
    acabar = TRUE;
    //
    // FIN DE QUIT
    //
  } else {
    // COMANDO DESCONOCIDO
    sprintf(bufferRespuesta, "Comando %s desconocido.", comando);
    codMensaje = CODIGO_COMANDO_DESCONOCIDO;
  }
  msg = constructorCodYString(codMensaje, bufferRespuesta, strlen(bufferRespuesta), IMPRIMIR_MENSAJES);
  // if (send(socket, msg, sizeof(msg), 0) < 0) {
  //   perror("servidorTCP: send: ");
  // }
  return msg;
}

void comandoList(char * arg, char * buf, int * cod);
void comandoGroup(char * arg, char * buf, int * cod);
void comandoNewNews(char * arg, char * buf, int * cod);
void comandoNewGroups(char * arg, char * buf, int * cod);
void comandoArticle(char * arg, char * buf, int * cod);
void comandoHead(char * arg, char * buf, int * cod);
void comandoBody(char * arg, char * buf, int * cod);
void comandoPost(char * arg, char * buf, int * cod);
