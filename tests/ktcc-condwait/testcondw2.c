#include <kitsune.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h> 


void * other(void * unused){
   while(1)
      kitsune_update("testkick");
}


void * testwait(void * unused){

   pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
   pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
   while(1)
   {
      kitsune_update("testwait");
      pthread_mutex_lock( &mutex );
      /*wait forever*/
      printf("\n\n*********2: thread waiting forever....*********************\n\n");
      fflush(stdout);
//      pthread_cond_wait( &cond, &mutex ); // this will never quiese.
      ktthreads_pthread_cond_wait( &cond, &mutex ); //this will break out and quiese.
      pthread_mutex_unlock( &mutex );
   }

   return NULL;
}

int main(void){

   pthread_t t;
   pthread_t k;
   int init = 0;
   int limitprint = 0;
   while(1){
      kitsune_update("main");
      if(!init){
         kitsune_pthread_create(&t, NULL, &testwait, NULL);
         kitsune_pthread_create(&k, NULL, &other, NULL);
         init = 1;
      }
      limitprint++;
      ktthreads_ms_sleep(100);
      if(limitprint%500 == 0){
         printf("2: main loop  ");
         fflush(stdout);
      }
   }

   return 0;
}

