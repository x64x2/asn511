#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#define  _S5LIB_C_
#include <as511_s5lib.h>

dlh_t *dlh_create ( int dl_type )
{
  dlh_t *dlh;
  dlh = Malloc(sizeof(dlh_t));
  dlh->f = NULL;
  dlh->l = NULL;
  dlh->dl_type = dl_type;
  return dlh;
}

dl_t *dl_create ( void )
{
  dl_t *dl;
  dl = Malloc(sizeof(dl_t));
  return dl;
}

dl_t *dlh_insert_last( dlh_t *dlh )
{
  dl_t *t = NULL;

  if( dlh != NULL ) {
    if( dlh->l == NULL )
      dlh->f = dlh->l = dl_create();
    else {
      t = dl_create();
      dlh->l->n = t;
      t->v = dlh->l;
      dlh->l = t;
    }
    return dlh->l;
  }
  return NULL;
}

dl_t *dlh_insert_first( dlh_t *dlh )
{
  dl_t *t = NULL;

  if( dlh != NULL ) {
    if( dlh->f == NULL )
      dlh->f = dlh->l = dl_create();
    else {
      t = dlh->f;
      dlh->f = dl_create();
      dlh->f->n = t;
      t->v = dlh->f;
    }
  }
  return dlh->f;
}

int   dl_insert_data( dlh_t *dlh, dl_t *dl, int dl_type, void *dl_data, size_t ds, void *udata )
{
  dl_t *dli;
  if( dlh != NULL && dl != NULL ) {
    if( dl_type == dlh->dl_type ) {
      for( dli = dlh->f; dli; dli = dli->n )
        if( dli == dl )
          break;
      if( dli != NULL && dl_data != NULL ) {
        dl->data = Malloc(ds);
        dl->udata = udata;
        memcpy( dl->data, dl_data, ds );
      }
      else
        return MEM_NO_DATA;
    }
    else
      return MEM_BAD_TYPE;
  }
  else
    return MEM_NO_DATA;

  return MEM_NO_ERROR;
}

void * dlh_delete( dlh_t *dlh, dl_t *dl, int dl_type, int (*usrfk)(void*), void *ud )
{
  dl_t *dli, *dlv, *dln;
  void *d = NULL;

  if( dlh == NULL )
    return NULL;

  if( dlh->dl_type != dl_type )
    return NULL;
  for( dli = dlh->f; dli; dli = dli->n ) {
    if( dli == dl )
      break;
  }

  if( dli == NULL )
    return NULL;

  d = dl->data;

  if( usrfk != NULL )
    if ( (*usrfk)(ud) != 0 )
      return NULL;

  if( dl->v == NULL && dl->n == NULL ) { 
    free( dl );                       
    dlh->f = dlh->l = NULL;
  }
  else {
    if( dl->v == NULL && dl->n != NULL ) { 
      dl = dl->n;
      Free(dl->v);
      dl->v = NULL;
      dlh->f = dl;
    }
    else {
      if( dl->v != NULL && dl->n == NULL ) { 
        dl = dl->v;
        Free(dl->n);
        dl->n = NULL;
        dlh->l = dl;
      }
      else { 
        dln = dl->n;
        dlv = dl->v;
        dln->v = dlv;
        dlv->n = dln;
        Free(dl);
      }
    }
  }
  return d;
}

#if 0
// Debug der Speicherverwaltung
int dl_print_data( dl_t *dl )
{
  fprintf(stderr,"Addr von dl        = %p\n", dl);
  if( dl ) {
    fprintf(stderr,"Addr von dl->n     = %p\n", dl->n);
    fprintf(stderr,"Addr von dl->v     = %p\n", dl->v);
    fprintf(stderr,"Addr von dl->data  = %p\n", dl->data);
  }
  return 0;
}
#endif
