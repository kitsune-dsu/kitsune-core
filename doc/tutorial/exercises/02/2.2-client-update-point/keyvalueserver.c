#define _GNU_SOURCE 
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <search.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <kitsune.h>

struct hsearch_data *store;

int init_listen_socket(int port)
{
  int rec_sock, status;
  struct sockaddr_in saddr;
  int i = 0, yes = 1;

  rec_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  assert(rec_sock > 0);

  memset(&saddr, 0, sizeof(struct sockaddr_in));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);

  status = setsockopt(rec_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (bind(rec_sock, (struct sockaddr *)&saddr, 
           sizeof(struct sockaddr_in)) < 0)
    abort();
  if (listen(rec_sock, 10) < 0)
    abort();

  return rec_sock;
}

void init_store(void){
  store = calloc(1, sizeof(struct hsearch_data));
  hcreate_r(100, store);
}

void handle_client(int client)
{
  const int line_size = 1000;
  char * line = calloc(sizeof(char), line_size);
  const char *delim = " \n";
  char *command = NULL, *arg1 = NULL, *arg2 = NULL, *savptr = NULL;

  while (1) {
    memset(line, 0, line_size);
    recv(client, line, line_size, 0);

    /* parse command */
    command = strtok_r(line, delim, &savptr);
    if (command == NULL)        /* client sent EOF */
      break;
    arg1 = strtok_r(NULL, delim, &savptr);
    if (strcmp("set", command) == 0)
      arg2 = strtok_r(NULL, delim, &savptr);

    /* handle */
    if (strcmp("set", command) == 0) {
      ENTRY e, *ret = NULL;
      e.key = strdup(arg1);
      e.data = strdup(arg2);
      hsearch_r(e, ENTER, &ret, store);
    } else if (strcmp("get", command) == 0) {
      ENTRY *ret = NULL, e;
      e.key = strdup(arg1);
      hsearch_r(e, FIND, &ret, store);
      if (ret == NULL)
        continue;
      sprintf(line, "%s\n", (char *)ret->data);
      send(client, line, strlen(line) + 1, 0);
    }
  }
  close(client);
}

int
main(int argc, char **argv)
{
  int server_sock = 0, client_sock = 0, 
    client_addrlen = sizeof(struct sockaddr_in);
  struct sockaddr_in client_addr;

  init_store();
  server_sock = init_listen_socket(5000);
  assert(server_sock > 0);
  
  while (1) {
    client_sock = accept(server_sock, &client_addr, 
                         &client_addrlen);
    if (client_sock < 0) {
      printf("%s\n", strerror(errno));
      abort();
    }
    handle_client(client_sock);
  }
  exit(0);
}
