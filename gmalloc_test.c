#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv) {
  unsigned *buffer = (unsigned *)malloc(sizeof(unsigned) * 100);
  unsigned i;

  for (i = 0; i < 200; i++) {
    buffer[i] = i;
  }

  for (i = 0; i < 200; i++) {
    printf ("%d  ", buffer[i]);
  }
}
