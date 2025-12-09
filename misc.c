#include "misc.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
/*

** Fichero: misc.c
** Autores:
 Alba Cruz García
*/
void strToUpper(char * str) {
  int i = 0;
  if (str != NULL) {
    while (str[i] != '\0') { //\0 para strings normales, \r\n para strings de NNTP
      str[i] = toupper(str[i]);
      i++;
    }
  } else {
    perror("strToUpper: String nulo");
  }
}


tipoMensaje * constructorCodYString(int codigo, char * msg, int msgSize, boolean print) {
  tipoMensaje * tmp = malloc(sizeof(tipoMensaje));
  if (msgSize > 512) {
    return NULL;
  }
  tmp->codRespuesta = codigo;
  strcpy(tmp->datos, msg);
  if (print) imprimirMensaje(tmp);
  return tmp;
}

void imprimirMensaje(tipoMensaje * msg) {
  if (msg != NULL && strlen(msg->datos) < 512) {
    printf("%" PRIu16 ": %s\n", msg->codRespuesta, msg->datos);
  } else {
    perror("imprimirMensaje: NULL");
  }
}

char * trim(char * str) {
  //https://stackoverflow.com/a/122721
  char * end;
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
  return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;

}

/* ----------------------------------------------------------------------------------------- */
int operarSobreSemaforo(int semaforo,int indice,short op,short nsops,short flg) {
/* ----------------------------------------------------------------------------------------- */
		struct sembuf sop = {indice,op,flg};
		return semop(semaforo,&sop,nsops);
}
