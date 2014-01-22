#ifndef PERF_H
#define PERF_H

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>

#define min(a, b) (((a) < (b)) ? (a) : (b)) 
#define max(a, b) (((a) > (b)) ? (a) : (b)) 

static double
log_perf(const char* fmt, ...) {
  struct timeval t;
  gettimeofday(&t, NULL);
  double dt = t.tv_sec + t.tv_usec / 1000000.;
  fprintf(stderr, "PERF(%.6f) > ", dt);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  fflush(stderr);
  return dt;
}

void 
internal_send(int socket, void * buffer, size_t size)
{
  int notsent = size, send_return = 0, sent = 0;

  while(notsent > 0 && 
	(send_return = send(socket, buffer + sent, notsent, 0)) > 0) {
    notsent -= send_return;
    sent += send_return;
  }
  if (send_return == -1) {
    perror("send: ");
    abort();
  }
}

void 
internal_recv(int socket, void * buffer, size_t size)
{
  int notread = size, recv_return = 0, read = 0;

  while(notread > 0 &&
	(recv_return = recv(socket, buffer + read, notread, 0)) > 0) {
    notread -= recv_return;
    read += recv_return;
  }
  if (recv_return == -1) {
    perror("recv: ");
    abort();
  }  
}

void syncup(int fd) {
  char s;
  internal_send(fd, &s, sizeof(char));
  internal_recv(fd, &s, sizeof(char));
}

void
internal_write(int socket, void * buffer, size_t size)
{
  int notsent = size, send_return = 0, sent = 0;

  while(notsent > 0 &&
	(send_return = write(socket, buffer + sent, notsent)) > 0) {
    notsent -= send_return;
    sent += send_return;
  }
  if (send_return == -1) {
    perror("write: ");
    abort();
  }
  return;
}

void
internal_read(int socket, void * buffer, size_t size)
{
  int notread = size, recv_return = 0, received = 0;

  while(notread > 0 &&
	(recv_return = read(socket, buffer + received, notread)) > 0) {
    notread -= recv_return;
    received += recv_return;
  }
  if (recv_return == -1) {
    perror("read: ");
    abort();
  }
}


#endif
