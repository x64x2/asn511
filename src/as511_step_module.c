#include <setjmp.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#define  _S5LIB_C_
#include <as511_s5lib.h>

int as511_step_module_destroy( td_t *td, int (*usrfk)(void*) )
{
  void *d;

  if( td != NULL && td->dlh != NULL && td->dlh->dl_type == DL_TYPE_STEP_MODULE ) {
    while( td->dlh->f != NULL ) {
      d = dlh_delete(td->dlh, td->dlh->l,DL_TYPE_STEP_MODULE,usrfk,td->dlh->l->udata);
      Free(d);
    }
    Free(td->dlh);
    td->dlh = NULL;
    return 0; 
  }
  return 1; 
}

int as511_step_module_create( td_t *td )
{
  if( td != NULL && td->dlh == NULL ) {
    td->dlh = dlh_create( DL_TYPE_STEP_MODULE );
    return 0;
  }
  return 1; 
}

int as511_step_module_insert_op( td_t *td, unsigned short type, unsigned short addr, void *udata )
{
  smd_u smd;
  dl_t  *dli;

  if( td->dlh != NULL ) {
    if( td->dlh->dl_type == DL_TYPE_STEP_MODULE ) {
      smd.t.type = type;
      smd.t.addr = addr; 
      dli = dlh_insert_last( td->dlh );
      if( dl_insert_data( td->dlh, dli, DL_TYPE_STEP_MODULE, &smd, sizeof(smd), udata ) > 0 ) {
        return 1; 
      }
    }
    return 0;
  }
  return 1; 
}

int as511_step_module_init( td_t *td )
{
  int rc;
  unsigned char ch;
  if( td == NULL ){
    return -1;
  }

  td->errnr = 0;
  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    if( protokoll_start( td, S5_DEBUG_INIT ) ) {
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      protokoll_stopp( td );
    }
  }
  return rc;
}

dl_t *as511_step_module_start( td_t *td, unsigned short offset, unsigned char bst_typ, unsigned char bst_nr )
{
  dl_t *dl;
  smd_u *smd;
  int rc;
  unsigned char ch;

  td->errnr = 0;

  if( td != NULL && td->dlh != NULL && td->dlh->f != NULL && td->dlh->dl_type == DL_TYPE_STEP_MODULE ) {
    dl = td->dlh->f;
    smd = DL_GET_DATA(smd_u, dl );
    if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
      if( protokoll_start( td, S5_DEBUG_START ) ) {
        schreibe_daten_v2(td, HI(offset));
        schreibe_daten_v2(td, LO(offset));
        schreibe_daten_v2(td, 0x01); 
        schreibe_daten_v2(td, 0x00);
        schreibe_daten_v2(td, bst_typ);
        schreibe_daten_v2(td, bst_nr);

        schreibe_byte_v2(td,0x10);
        schreibe_byte_v2(td,UCHAR(smd->t.type));
        if( smd->t.type != DEBUG_MODULE_NOPAR &&
            smd->t.type != DEBUG_MODULE_LOAD  &&
            smd->t.type != DEBUG_MODULE_LOAD_LARGE ) {
          schreibe_daten_v2(td,HI(smd->t.addr));
          schreibe_daten_v2(td,LO(smd->t.addr));
        }
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, EOT);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ACK, 1);

        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);

        as511_read_data( td );

        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);
        protokoll_stopp( td );
      }
      copy_module_data( td, 3, smd );
      return dl;
    }
    else {
      td->errnr = rc;
      return NULL;
    }
  }
  return NULL;
}

dl_t *as511_step_module_continue( td_t *td, dl_t *dl )
{
  smd_u *smd;
  unsigned char ch;
  dl_t *dli;

  td->errnr = 0;
  if( td == NULL || td->dlh == NULL || dl == NULL || td->dlh->dl_type != DL_TYPE_STEP_MODULE) {
    return NULL;
  }

  for( dli = td->dlh->f; dli; dli = dli->n ) {
    if( dl == dli)
      break;
  }

  if( dli == NULL )
    return NULL;

  if( (smd = DL_GET_DATA(smd_u, dl )) != NULL ) {
    if( sigsetjmp(td->env, 1) == 0 ) {
      if( protokoll_start( td, S5_DEBUG_CONTINUE ) ) {
        schreibe_byte_v2(td,0x10);
        schreibe_byte_v2(td,UCHAR(smd->t.type));
        if( smd->t.type != STATUS_MODULE_NOPAR &&
            smd->t.type != STATUS_MODULE_LOAD  &&
            smd->t.type != STATUS_MODULE_LOAD_LARGE ) {
            schreibe_daten_v2(td,HI(smd->t.addr));
            schreibe_daten_v2(td,LO(smd->t.addr));
        }
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, EOT);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ACK, 1);

        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td, DLE );
        schreibe_byte_v2(td, ACK );

        as511_read_data( td );

        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);
        protokoll_stopp( td );
      }
      copy_module_data( td, 1, smd );
    }
  }
  return dl;
}

int as511_step_module_stop( td_t *td )
{
  int rc;
  unsigned char ch;
  if( td == NULL ){
    return -1;
  }

  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    schreibe_byte_v2(td, STX);
    lese_byte_v2(td, &ch, DLE, 1);
    lese_byte_v2(td, &ch, ACK, 1);
    schreibe_daten_v2(td, S5_ONLINE_STOP );      
    schreibe_byte_v2(td, DLE);
    schreibe_byte_v2(td, ETX);
    lese_byte_v2(td, &ch, DLE, 1);
    lese_byte_v2(td, &ch, ACK, 1);

    protokoll_stopp( td );
  }
  td->errnr = rc;
  return 0;
}
