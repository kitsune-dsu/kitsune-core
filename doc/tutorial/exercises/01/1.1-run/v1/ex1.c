#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <kitsune.h>

int
main(int argc, char **argv)
{
  printf("Hello, world! from version 1\n");
  kitsune_update("main");
  exit(0);
}
