#include <setjmp.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#define  _S5LIB_C_
#include <as511_s5lib.h>

int as511_status_var_destroy( td_t *td, int (*usrfk)(void*) )
{
  void *d;

  if( td != NULL && td->dlh != NULL && td->dlh->dl_type == DL_TYPE_STATUS_VAR ) {
    while( td->dlh->f != NULL ) {
      d = dlh_delete(td->dlh, td->dlh->l,DL_TYPE_STATUS_VAR,usrfk,td->dlh->l->udata);
      Free(d);
    }
    Free(td->dlh);
    td->dlh = NULL;
    return 0; 
  }
  return 1; 
}

int as511_status_var_create( td_t *td )
{
  if( td != NULL && td->dlh == NULL ) {
    td->dlh = dlh_create( DL_TYPE_STATUS_VAR );
    return 0; 
  }
  return 1; 
}

dl_t *as511_status_var_insert_type( td_t *td, unsigned char type, unsigned short addr, int (*usrfk)(void*), void *udata )
{

  svd_u svd;
  dl_t  *dl = NULL;

  svd.t.type = type;
  svd.t.addr = addr;

  if( td->dlh->dl_type == DL_TYPE_STATUS_VAR ) {
    dl = dlh_insert_last( td->dlh );
    if( dl_insert_data( td->dlh, dl, DL_TYPE_STATUS_VAR, &svd, sizeof(svd_u), udata ) > 0 ) {
      dlh_delete( td->dlh, dl, DL_TYPE_STATUS_VAR, usrfk, udata);
      return NULL;
    }
  }
  return dl;
}

int as511_status_var_start( td_t *td )
{
  svd_u *svd;
  dl_t  *dl;
  int rc;
  unsigned char ch;

  if( td == NULL || td->dlh == NULL )
    return 0;

  td->errnr = 0;

  if( td->dlh->f == NULL ) {       
    td->errnr = STATUS_NO_DATA;
    return 0;
  }

  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    if( protokoll_start( td, S5_STATUS_VAR ) ) {
      schreibe_daten_v2(td, 0x00); 
      schreibe_daten_v2(td, 0x00);
      schreibe_daten_v2(td, 0x00); 
      schreibe_daten_v2(td, 0x00);
      schreibe_daten_v2(td, 0x10); 
      schreibe_daten_v2(td, 0x3F); 
      for( dl = td->dlh->f; dl != NULL; dl = dl->n ) {
        svd = DL_GET_DATA(svd_u, dl );
        schreibe_byte_v2(td,0x10);
        schreibe_byte_v2(td,UCHAR(svd->t.type));
        schreibe_byte_v2(td,HI(svd->t.addr));
        schreibe_byte_v2(td,LO(svd->t.addr));
      }
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);

      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);
      lese_byte_v2(td, &ch, 0x10, 1); 
      lese_byte_v2(td, &ch, 0x10, 1); 
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ETX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);
    }
  }
  else {
    td->errnr = rc;
    return 0;
  }

  return 1;
}

static int status_var_run( td_t *td, unsigned char bef )
{
  unsigned char ch;

  schreibe_byte_v2(td, STX);
  lese_byte_v2(td, &ch, DLE, 1);
  lese_byte_v2(td, &ch, ACK, 1);
  schreibe_byte_v2(td, bef);     
  schreibe_byte_v2(td, DLE);
  schreibe_byte_v2(td, ETX);
  lese_byte_v2(td, &ch, DLE, 1);
  lese_byte_v2(td, &ch, ACK, 1);

  return 1;
}

int as511_status_var_run( td_t *td )
{
  unsigned char ch;
  int rc;
  svd_u *svd;
  dl_t  *dl;
  unsigned int index = 0;

  td->errnr = 0;

  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    if( status_var_run( td, S5_ONLINE_START ) ) {
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);

      lese_byte_v2(td, &ch, 0x00, 1); 
      lese_byte_v2(td, &ch, 0x00, 0); 
      lese_byte_v2(td, &ch, 0x00, 1); 

      index = as511_read_data( td );

      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);
      if( td->dlh != NULL ) {
        index = 0;
        for( dl = td->dlh->f ; dl != NULL; dl = dl->n ) {
          svd = DL_GET_DATA(svd_u, dl );
          switch( svd->t.type ) {
            case STATUS_VAR_PAE:
            case STATUS_VAR_PAA:
            case STATUS_VAR_MERKER:
              memcpy(&svd->t4.status_0, &td->mem[index], 4 );
              index += 4;
              break;
            case STATUS_VAR_ZAEHLER:
            case STATUS_VAR_DATEN:
              memcpy(&svd->t6.status_0, &td->mem[index], 6 );
              index += 6;
  #if __BYTE_ORDER == __LITTLE_ENDIAN
              swab(&svd->t6.w.d.wert,&svd->t6.w.d.wert,sizeof(short));
  #endif
              break;
          }
        }
      }
    }
  }
  else {
    td->errnr = rc;
  }
  return 0;
}

int as511_status_var_stop( td_t *td )
{
  int rc;

  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    if( status_var_run( td, S5_ONLINE_STOP ) ) { // 0x81
      protokoll_stopp( td );
      return 1;
    }
  }
  td->errnr = rc;
  return 0;
}

