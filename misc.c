#include "misc.h"
void strToUpper(char * str) {
  int i = 0;
  if (str != null) {
    while (str[i] != '\0') {
      toupper(str[i]);
    }
  } else {
    perror("strToUpper: String nulo");
  }
}
