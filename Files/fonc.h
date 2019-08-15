#ifndef _FONC_H_
#define _FONC_H_

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>

/*********** Data Type ***********/
typedef enum
  {
  STATE_WAIT,
  STATE_MULT,
  STATE_NEXT,//Pour changement d'iteration V2
  STATE_PRINT
  } State;


typedef struct
  {
  State state;
  int ** pendingMult;
  pthread_cond_t cond;
  pthread_mutex_t mutex;
  size_t nbIterations;
  int sizeM1[2];
  int sizeM2[2];
  int ** m1;
  int ** m2;

  int ** result;
  } Product;

typedef struct
  {
  	int lresult;
  	int cresult;
  }multParam;

/*********** Data structure ***********/

Product prod;

/************ Functions ***************/

void initPendingMult(Product * prod);
void * mult(void * paramTemp);
void getLineMmap(char*file,int nLine,char*line);
void setSize(char * file);
void parseString(char*input,int *tabParsed, int tabSize);
void setProduct(char*file);

#endif
