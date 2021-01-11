#include <sys/errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/socket.h>
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
int semaforos;
boolean acabar;
boolean acabarPost = FALSE;
char * rutaGrupoSeleccionado = NULL;
tipoMensaje * procesarComando(tipoMensaje * mensajeEntrada);
tipoMensaje * procesarPost(tipoMensaje * mensajeEntrada);
void logMessage(char * string);
int main(int argc, char * argv) {
  // socketListen = ls_tcp, socketTCP = s_tcp en las practicas.
  struct linger linger;
  linger.l_onoff = 1;
  linger.l_linger = 1;
  int socketListenTCP, socketTCP, socketUDP, socketMayor, nElegido;
  int tamMensaje;
  tipoMensaje * mensajeRecibido = malloc(sizeof(tipoMensaje));
  tipoMensaje * mensajeRespuesta;
  char host[96], mensajeLog[128];
  struct sockaddr_in servidor, cliente, clienteUDP;
  struct sigaction sigCld;
  socklen_t tamSocket = sizeof(struct sockaddr_in);
  //Union de semaforo para que funcione en nogal.
  union semun {
					int val;
					struct semid_ds *buf;
					unsigned short  *array;
	  } unionSemaforo;
  //Damos valores a los sockaddr
  servidor.sin_family = AF_INET;
  servidor.sin_addr.s_addr = INADDR_ANY;
  servidor.sin_port = htons(PUERTO);
  //Instanciamos las señales que cambiamos
  sigCld.sa_handler = SIG_IGN;
  //sigCld.sa_flags = SA_NOCLDWAIT;
  sigemptyset(&sigCld.sa_mask);
  sigaction(SIGCLD, &sigCld, NULL);
  // Declaramos los semaforos
  unionSemaforo.val = 1;
  if (semctl(semaforos, SEMAFORO_LOG, SETVAL, unionSemaforo) == -1) {
    fprintf(stderr, "SEMAFORO_LOG: Declaración");
  }
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
  printf("DAEMON SERVIDOR NNTP\n");
  printf("-------------------------------------\n\n");
  //Recibimos el mensaje
  //setpgrp();
  while (acabar != TRUE) {
    fd_set socketSet;
    pid_t serverPID;
    socketMayor = (socketUDP > socketListenTCP)? socketUDP : socketListenTCP;
    FD_ZERO(&socketSet);
    FD_SET(socketListenTCP, &socketSet);
    FD_SET(socketUDP, &socketSet);
    nElegido = select(socketMayor+1, &socketSet, (fd_set *)0, (fd_set *)0, NULL);
    serverPID = fork();
    switch(serverPID) {
    case -1:
        fprintf(stderr, "fork: Error al crear hijo.");
        close(socketUDP);
        close(socketListenTCP);
        close(socketTCP);
        exit(EXIT_ERR_GENERICO);
        break;
    case 0:
        break;
    default:
      if (FD_ISSET(socketListenTCP, &socketSet)) {
        // Accept al socket TCP
        if ((socketTCP = accept(socketListenTCP, (struct sockaddr *)&cliente, &tamSocket)) == -1) {
          fprintf(stderr, "socketTCP: Error de accept.");
          close(socketUDP);
          close(socketListenTCP);
          close(socketTCP);
          exit(EXIT_ERR_GENERICO);
        }
        getnameinfo((struct sockaddr *)&cliente, tamSocket, host, tamSocket, NULL, 0, 0);
        printf("Conectado a %s por TCP (puerto %d).\n", host, htons(cliente.sin_port));
        while (acabar != TRUE) {
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
          switch (mensajeRespuesta->codRespuesta) {
            case CODIGO_POST_INICIO:
              tamMensaje = recv(socketTCP, mensajeRecibido, sizeof(tipoMensaje), 0);
              if (tamMensaje <= 0) {
                fprintf(stderr, "socketTCP: Error de recv.");
                close(socketListenTCP);
                close(socketTCP);
                exit(EXIT_ERR_GENERICO);
              }
              mensajeRespuesta = procesarPost(mensajeRecibido);
              if (send(socketTCP, mensajeRespuesta, sizeof(tipoMensaje), 0) < 0) {
                perror("servidorTCP: send: ");
              }
              break;
            case CODIGO_DESPEDIDA:
              break;
            default:
              break;
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
        getnameinfo((struct sockaddr *)&servidor, tamSocket, host, tamSocket, NULL, 0, 0);
        printf("Conectado a %s por UDP (puerto %d).\n", host, htons(servidor.sin_port));
        sprintf(mensajeLog, "Conectado a %s (IP: %s) por UDP (puerto %d)", host, inet_ntoa(servidor.sin_addr), htons(servidor.sin_port));
        logMessage(mensajeLog);
        while (acabar != TRUE) {
          tamMensaje = recvfrom(socketUDP, mensajeRecibido, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, &tamSocket);
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
          switch (mensajeRespuesta->codRespuesta) {
            case CODIGO_POST_INICIO:
              tamMensaje = recvfrom(socketUDP, mensajeRecibido, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, &tamSocket);
              if (tamMensaje <= 0){
                fprintf(stderr, "socketTCP: Error de recv.");
                close(socketListenTCP);
                close(socketTCP);
                exit(EXIT_ERR_GENERICO);
              }
              mensajeRespuesta = procesarPost(mensajeRecibido);

              if (sendto(socketUDP, mensajeRespuesta, sizeof(tipoMensaje), 0, (struct sockaddr *)&servidor, tamSocket) < 0) {
                perror("servidorUDP: sendto");
              }
              break;
            case CODIGO_DESPEDIDA:
              break;
            default:
              break;
          }
          free(mensajeRespuesta);
        }
        shutdown(socketUDP, SHUT_RDWR);
        close(socketUDP);
      }
      exit(EXIT_CORRECTO);
    }
  }
  shutdown(socketListenTCP, SHUT_RDWR);
  close(socketListenTCP);
  shutdown(socketTCP, SHUT_RDWR);
  close(socketTCP);
  shutdown(socketUDP, SHUT_RDWR);
  close(socketUDP);
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
  char argumentos[256] = {0}, bufferLinea[256], grupo[32] = {0}, fecha[32] = {0}, mensajeLog[128], rutaCarpeta[64] = {0}, rutaArchivo[80] = {0};
  char * comando, *saveptr, *saveptr2, *tmp;
  DIR * carpeta;
  FILE * archivoALeer;
  struct dirent * entry, * ultimaEntry, * firstEntry = NULL;
  int primerArticulo, ultimoArticulo;
  size_t tamArchivo;
  memcpy(bufferMensaje, mensajeEntrada->datos, TAM_BUFFER);
  sprintf(mensajeLog, "COMANDO RECIBIDO: %s", bufferMensaje);
  logMessage(mensajeLog);
  comando = strtok_r(bufferMensaje, " ", &saveptr);
  if (comando == NULL || strlen(comando) == 0) comando = bufferMensaje; // Comandos sin argumentos
  strToUpper(comando);
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
    int contadorEspacio = 0;
    while ((comando = strtok_r(NULL, ".", &saveptr)) != NULL) {
      sprintf(grupo, "/%s", trim(comando));
      contador++;
    }

    while ((comando = strtok_r(NULL, " ", &saveptr)) != NULL) {
      sprintf(argumentos, "%s", trim(comando));
      if (contadorEspacio == 0) sprintf(argumentos + strlen(argumentos), " ");
      contadorEspacio++;
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
    sprintf(rutaCarpeta, "%s", RUTA_ARTICULOS);
    //Parseamos
    while ((comando = strtok_r(NULL, ".", &saveptr)) != NULL) {
      trim(comando);
      sprintf(rutaCarpeta + strlen(rutaCarpeta), "/%s", comando);
      sprintf(argumentos + strlen(argumentos), "%s.", comando);
      contador++;
    }
    if (contador == 0) {
      codMensaje = CODIGO_COMANDO_SINTAXIS;
      sprintf(bufferRespuesta, "GROUP: No se encontraron suficientes tokens. Recordaste introducir el nombre de grupo?");
    } else {
      //Borramos el último punto de argumentos.
      argumentos[strlen(argumentos)-1] = '\0';
      archivoALeer = fopen(RUTA_ARCHIVO_GRUPOS, "r");
      while (!feof(archivoALeer)) {
        fgets(bufferLinea, sizeof(bufferLinea), archivoALeer);
        //Comparamos el nombre del grupo
        tmp = strtok_r(bufferLinea, " ", &saveptr2);
        trim(tmp);
        if (!(strcmp(tmp, argumentos))) { //El grupo se encuentra en el archivo de grupos (existe).
          if (rutaGrupoSeleccionado == NULL) rutaGrupoSeleccionado = malloc(sizeof(char) * 128);
          sprintf(rutaGrupoSeleccionado, "%s", rutaCarpeta);
          // Los siguientes dos tokens son el ultimo articulo y el primero
          tmp = strtok_r(NULL, " ", &saveptr2);
          ultimoArticulo = atoi(tmp);
          tmp = strtok_r(NULL, " ", &saveptr2);
          primerArticulo = atoi(tmp);
        }
      }
      sprintf(bufferRespuesta, "%d %d %d %s", ultimoArticulo-primerArticulo, primerArticulo, ultimoArticulo, argumentos);
      codMensaje = CODIGO_GRUPO_SELECCIONADO;
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
    codMensaje = CODIGO_POST_INICIO;
    sprintf(bufferRespuesta, "Subiendo un artículo; finalize con una línea que solo contenga un punto");
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
    sprintf(bufferRespuesta, "Comando \"%s\" desconocido.", comando);
    codMensaje = CODIGO_COMANDO_DESCONOCIDO;
  }
  sprintf(mensajeLog, "RESPUESTA: %d - %s", codMensaje, bufferRespuesta);
  logMessage(mensajeLog);
  msg = constructorCodYString(codMensaje, bufferRespuesta, strlen(bufferRespuesta), IMPRIMIR_MENSAJES);
  return msg;
}

void logMessage(char * string) {
  FILE * archivoLog;
  time_t t = time(NULL);
  struct tm * fechaHora = localtime(&t);
  char * strFechaHora = asctime(fechaHora);
  operarSobreSemaforo(semaforos, SEMAFORO_LOG, WAIT, 1, 0);
  archivoLog = fopen("nntp.log", "a");
  if (archivoLog == NULL) return;
  fprintf(archivoLog, "%.*s: %s" , strlen(strFechaHora)-1, strFechaHora, string);
  fclose(archivoLog);
  operarSobreSemaforo(semaforos, SEMAFORO_LOG, SIGNAL, 1, 0);
}

tipoMensaje * procesarPost(tipoMensaje * mensajeEntrada) {
  tipoMensaje * msg;
  int i, j, numUltimoArticulo, codRespuesta;
  char bufferRespuesta[512], copiaLinea[512] = {0}, copiaMensaje[512], * linea, * saveptr, * saveptr2;
  char rutaArchivo[96], subject[96], contenido[512] = {0}, textoGrupo[48], fechaFormatoNNTP[96], fechaLegible[96], hostname[96];
  FILE * archivoArticulo, * archivoGrupo;
  time_t t = time(NULL);
  struct tm fechaHora = *localtime(&t);
  strftime(fechaFormatoNNTP, 96, "%C%m%d %H%M%S", &fechaHora);
  strftime(fechaLegible, 96, "%a, %e %b %Y %T %z %Z", &fechaHora);
  gethostname(hostname, 96);
  strcpy(copiaMensaje, mensajeEntrada->datos);
  sprintf(rutaArchivo, "%s/", RUTA_ARTICULOS);
  for (i = 0, linea = strtok_r(copiaMensaje, "\n", &saveptr); linea != NULL; linea = strtok_r(NULL, "\n", &saveptr), i++) {
    sprintf(copiaLinea, "%s", linea); //Copiamos para hacer más parsing de una sola linea.
    if (i == 0) { // Línea del grupo.
      sprintf(textoGrupo, "%s", copiaLinea);
      trim(textoGrupo);
      for (j = 0; j < strlen(copiaLinea); j++) {
        //Reemplazamos los . por / para crear una ruta. Se puede hacer con strtok, pero bueno...
        sprintf(rutaArchivo+strlen(rutaArchivo), "%c", (copiaLinea[j] == '.')? '/' : copiaLinea[j]);
      }
    } else if (i == 1) {
      sprintf(subject, "%s", copiaLinea);
    } else {
      sprintf(contenido+strlen(contenido), "%s\n", copiaLinea);
    }
    if (!(strcmp(trim(linea), "."))) break;
  }
  fflush(stdout);
  //A partir de aqui usamos la variable "Linea" para guardar los tokens.
  if ((archivoGrupo = fopen(RUTA_ARCHIVO_GRUPOS, "r+")) == NULL) {
    codRespuesta = CODIGO_ERROR_GENERICO;
    sprintf(bufferRespuesta, "No se pudo abrir el archivo de grupos.");
  } else {
    while (!feof(archivoGrupo)) { //Acabamos si acaba el archivo o hemos encontrado elg rupo
      fgets(copiaLinea, sizeof(copiaLinea), archivoGrupo);
      linea = strtok_r(copiaLinea, " ", &saveptr2); //El primer token es el del grupo.
      if (!(strcmp(trim(linea), textoGrupo))) break;
    }
    if (feof(archivoGrupo)) {
      codRespuesta = CODIGO_GRUPO_INEXISTENTE;
      sprintf(bufferRespuesta, "El grupo %s no existe.", textoGrupo);
    } else {
      linea = strtok_r(NULL, " ", &saveptr2); //Este token es el del último artículo y es el que queremos.
      numUltimoArticulo = atoi(linea);
      sprintf(rutaArchivo+strlen(rutaArchivo), "/%d", numUltimoArticulo+1);
      archivoArticulo = fopen(rutaArchivo, "w+");
      if (archivoArticulo == NULL) {
        codRespuesta = CODIGO_ERROR_SUBIDA;
        sprintf(bufferRespuesta, "No se pudo crear el fichero del nuevo artículo.");
      } else {
        fprintf(archivoArticulo, "Newsgroup: %s\nSubject: %s\nDate: %s %s\nMessage-ID: <%d@%s>\n\n%s", textoGrupo, subject, fechaFormatoNNTP, fechaLegible, numUltimoArticulo+1, hostname, contenido);
        codRespuesta = CODIGO_POST_CORRECTO;
        sprintf(bufferRespuesta, "Post subido correctamente");
      }
    }
  }
  fclose(archivoGrupo);
  fclose(archivoArticulo);
  msg = constructorCodYString(codRespuesta, bufferRespuesta, strlen(bufferRespuesta), FALSE);
  return msg;
}
