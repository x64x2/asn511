#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#define  _S5LIB_C_
#include <as511_s5lib.h>


int as511_ctrl_output_insert_op( td_t *td, byte_t addr, byte_t value, void *udata )
{
  dl_t  *dli;
  cop_t  cop;

  if( td != NULL && td->dlh != NULL ) {
    if( td->dlh->dl_type == DL_TYPE_CTRL_OUTPUT ) {
      cop.addr  = addr;  
      cop.value = value; 

      dli = dlh_insert_last( td->dlh );
      if( dl_insert_data( td->dlh, dli, DL_TYPE_CTRL_OUTPUT, &cop, sizeof(cop_t), udata ) > 0 )
        return 1; 
    }
    return NO_ERROR;
  }
  return 1; 
}

int as511_ctrl_output_init( td_t *td )
{
  int rc = NO_ERROR;
  unsigned char ch;

  td->errnr = NO_ERROR;

  if( (rc = sigsetjmp(td->env, 1 )) == NO_ERROR ) {
    if( protokoll_start( td, S5_CTRL_OUTPUT_INIT ) ) {
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);
      lese_byte_v2(td, &ch, 0, 0);
#if 1
      switch( ch ) {
        case 0x10:
          lese_byte_v2(td, &ch, 0x10, 1);
          td->errnr = NO_ERROR;
          break;
        case 0x12:
          td->errnr = ERROR_AG_RUNING;
          break;
        default:
          td->errnr = CHAR_UNKNOWN;
          break;
      }
#else
      if( ch == 0x10 )
        lese_byte_v2(td, &ch, 0x10, 1);
      else if( ch == 0x12 )
        td->errnr = ERROR_AG_RUNING;
      else
        td->errnr = CHAR_UNKNOWN;
#endif
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ETX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);
    }
  }
  else {
    td->errnr = rc;
  }
  return (td->errnr) ? 1:NO_ERROR;
}

copbl_t *as511_ctrl_output_start( td_t *td )
{
  int     index;
  byte_t  ch;
  cop_t   *c;
  copbl_t *bl = NULL;
  dl_t  *dl;

  index  = 0;
  td->errnr = 0;

  if( sigsetjmp(td->env, 1) == NO_ERROR ) {
    if( protokoll_start( td, S5_CTRL_OUTPUT ) ) {
      for( dl = td->dlh->f; dl != NULL; dl = dl->n ) {
        c = DL_GET_DATA(cop_t, dl );
        schreibe_daten_v2(td, c->addr );
        schreibe_daten_v2(td, c->value );
      }
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);
      lese_byte_v2(td, &ch, 0, 0);
      if( ch == FS ) {
        // Falls eine Adresse angegeben, die nicht ansprechbar ist
        // werden in der Variabeln "badlst" die fehlerhaften Adressen
        // eingetragen.
        index = as511_read_data( td );

        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);
        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);
        lese_byte_v2(td, &ch, DC2, 1);
      }
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ETX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);

      if( index ) {
        bl = Malloc(sizeof(copbl_t));
        bl->badlst = Malloc(index);
        bl->badlstsize = index;
        memcpy(bl->badlst, td->mem, index);
        td->errnr = CTRL_OUTP_BADLST;
      }
    }
  }
  return bl;
}

int as511_ctrl_output_stop( td_t *td )
{
  int rc;
  byte_t ch;

  if( (rc = sigsetjmp(td->env, 1 )) == NO_ERROR ) {
    schreibe_byte_v2(td, STX);
    lese_byte_v2(td, &ch, DLE, 1);
    lese_byte_v2(td, &ch, ACK, 1);
    schreibe_byte_v2(td, S5_ONLINE_STOP);
    schreibe_byte_v2(td, DLE);
    schreibe_byte_v2(td, ETX);
    lese_byte_v2(td, &ch, DLE, 1);
    lese_byte_v2(td, &ch, ACK, 1);
    protokoll_stopp( td );
    return NO_ERROR;
  }
  td->errnr = rc;
  return 1;
}

int as511_ctrl_output_destroy( td_t *td, int (*usrfk)(void*) )
{
  void *d;

  if( td != NULL && td->dlh != NULL && td->dlh->dl_type == DL_TYPE_CTRL_OUTPUT ) {
    while( td->dlh->f != NULL ) {
      d = dlh_delete(td->dlh, td->dlh->l,DL_TYPE_CTRL_OUTPUT,usrfk,td->dlh->l->udata);
      Free(d);
    }
    Free(td->dlh);
    td->dlh = NULL;
    return NO_ERROR; 
  }
  return 1; 
}

int as511_ctrl_output_create( td_t *td )
{
  if( td != NULL && td->dlh == NULL ) {
    td->dlh = dlh_create( DL_TYPE_CTRL_OUTPUT );
    return NO_ERROR; 
  }
  return 1; 
}

void as511_ctrl_output_bl_free( td_t *td, copbl_t *bl )
{
  if( td->dlh->dl_type == DL_TYPE_CTRL_OUTPUT  && bl != NULL ) {
    Free(bl->badlst);
    Free(bl);
  }
}
