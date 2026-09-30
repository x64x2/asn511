#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include "as511_s5lib.h"
#include <as511_mem.h>

#define MEM_TEST 0
int MallocZaehler;

MD md;

void *Malloc( size_t size )
{
  void *p;
#if defined MEM_TEST && MEM_TEST > 0
  ML *mli, *t;
#endif
  if( (p = malloc(size)) == NULL )
  {
    perror("Out of Memory");
    abort();
  }

  memset(p,0x00,size);
  MallocZaehler++;
#if defined MEM_TEST && MEM_TEST > 0
  if( ++md.debug ) {
    if( (mli = malloc(sizeof(ML))) != NULL ) {
      memset(mli,0x00,sizeof(ML));
      mli->ptr = (int)p;
      mli->size = size;
      printf("%8d",MallocZaehler);
      printf("Malloc allocate size %6d bytes at %p\n", size, p );
      if( md.mlf == NULL ) {
        md.mll = md.mlf = mli;
      }
      else {
        t = md.mlf;
        mli->n = t;
        t->v = mli;
        md.mlf = mli;
      }
    }
  }
#endif
  return p;
}

void free( void *p )
{
#if defined MEM_TEST && MEM_TEST > 0
  ML *mli;
#endif
  if( p == NULL )
    return;
#if defined MEM_TEST && MEM_TEST > 0
  for( mli = md.mlf; mli; mli = mli->n ) {
    if( p == (void*)mli->ptr ) {
      mli->isfree = 1;
      printf("Free %6d byted at %p\n",mli->size, p );
      break;
    }
  }
#endif
  MallocZaehler--;
  free(p);
}
