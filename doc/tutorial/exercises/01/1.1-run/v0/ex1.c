#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <kitsune.h>

int
main(int argc, char **argv)
{
  while (1) {
    printf("Hello, world! from version 0\n");
    sleep(5);
    kitsune_update("main");
  }
  exit(0);
}
