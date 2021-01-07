#include "misc.h"
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

tipoMensaje * constructorCodRespuesta(int codigo) {
  tipoMensaje * msg = malloc(sizeof(tipoMensaje));
  //TODO: Error handling
  msg->codRespuesta = (uint16_t)codigo;
  msg->datos[0] = '\r';
  msg->datos[1] = '\n';
  msg->datos[2] = '\0';
  return msg;
}

tipoMensaje * constructorCodYString(int codigo, char * msg, int msgSize, boolean print) {
  tipoMensaje * tmp = malloc(sizeof(tipoMensaje));
  if (msgSize > 512) {
    return NULL;
  }
  tmp->codRespuesta = codigo;
  strcpy(tmp->datos, msg);
  if (print) imprimirMensaje(tmp);
  //TODO: Actualmente usamos \0 por comodidad, cambiar antes de enviar
  // if (msgSize > 509) {
  //   tmp->datos[msgSize-3] = '\r';
  //   tmp->datos[msgSize-2] = '\n';
  //   tmp->datos[msgSize-1] = '\0';
  // } else {
  //   //TODO: quizas sea +1 +2
  //   tmp->datos[msgSize] = '\r';
  //   tmp->datos[msgSize+1] = '\n';
  //   tmp->datos[msgSize+2] = '\0';
  // }
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
