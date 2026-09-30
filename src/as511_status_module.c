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


int as511_status_module_destroy( td_t *td, int (*usrfk)(void*) )
{
  void *d;

  if( td != NULL && td->dlh != NULL && td->dlh->dl_type == DL_TYPE_STATUS_MODULE ) {
    while( td->dlh->f != NULL ) {
      d = dlh_delete(td->dlh, td->dlh->l,DL_TYPE_STATUS_MODULE,usrfk,td->dlh->l->udata);
      Free(d);
    }
    Free(td->dlh);
    td->dlh = NULL;
    return 0; 
  }
  return 1; 
}

int as511_status_module_create( td_t *td )
{
  if( td != NULL && td->dlh == NULL ) {
    td->dlh = dlh_create( DL_TYPE_STATUS_MODULE );
    return 0; 
  }
  return 1; 
}

int as511_status_module_insert_op( td_t *td, unsigned short type, unsigned short addr, void *udata )
{
  smd_u smd;
  dl_t  *dli;

  if( td->dlh != NULL ) {

    if( td->dlh->dl_type == DL_TYPE_STATUS_MODULE ) {
      smd.t.type = type; 
      smd.t.addr = addr; 

      dli = dlh_insert_last( td->dlh );
      if( dl_insert_data( td->dlh, dli, DL_TYPE_STATUS_MODULE, &smd, sizeof(smd), udata ) > 0 )
        return 1; 
    }
    return NO_ERROR; 
  }
  return 1;
}

int as511_status_module_start( td_t *td, unsigned short offset,
                               unsigned char bst_typ, unsigned char bst_nr )
{
  int rc;
  unsigned char ch;
  smd_u *smd;
  dl_t  *dl;

  td->errnr = NO_ERROR;

  if( td != NULL && td->dlh != NULL && td->dlh->f != NULL && td->dlh->dl_type == DL_TYPE_STATUS_MODULE ) {
    if( (rc = sigsetjmp(td->env, 1 )) == NO_ERROR ) {
      if( protokoll_start( td, S5_STATUS_BST ) ) {
        schreibe_daten_v2(td, HI(offset));
        schreibe_daten_v2(td, LO(offset));
        schreibe_daten_v2(td, 0x01); 
        schreibe_daten_v2(td, 0x00); 
        schreibe_daten_v2(td, bst_typ);
        schreibe_daten_v2(td, bst_nr);

        for( dl = td->dlh->f; dl != NULL; dl = dl->n ) {
          smd = DL_GET_DATA(smd_u, dl );
          schreibe_byte_v2(td,0x10);
          schreibe_byte_v2(td,UCHAR(smd->t.type));
          if( smd->t.type != STATUS_MODULE_NOPAR &&
              smd->t.type != STATUS_MODULE_LOAD  &&
              smd->t.type != STATUS_MODULE_LOAD_LARGE ) {
                schreibe_daten_v2(td,HI(smd->t.addr));
                schreibe_daten_v2(td,LO(smd->t.addr));
           }
        }
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, EOT);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ACK, 1);

        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);
        lese_byte_v2(td, &ch, 0, 0); 
        if( ch == 0x10 )
          lese_byte_v2(td, &ch, 0x10, 1);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ETX, 1);
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);
      }
    }
    else {
      td->errnr = rc;
      return 1;
    }
  }
  return NO_ERROR;
}

static int status_module_run( td_t *td, unsigned char bef )
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

int as511_status_module_run( td_t *td )
{
  unsigned char ch;
  int rc;
  unsigned int index = 0;

  smd_u *smd;
  dl_t  *dl;

  td->errnr = 0;
  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    if( status_module_run( td, S5_ONLINE_START ) ) { 
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);

      lese_byte_v2(td, &ch, 0x00, 1); 
      lese_byte_v2(td, &ch, 0x00, 0); 
      lese_byte_v2(td, &ch, 0x00, 1); 

      index = as511_read_data( td );

      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, ACK);
      for( index = 0,dl = td->dlh->f; dl != NULL; dl = dl->n ) {
        smd = DL_GET_DATA(smd_u, dl );
        index = copy_module_data( td, index,  smd );
      }
    }
  }
  else {
    td->errnr = rc;
    return 1;
  }

  return 0;
}

int as511_status_module_stop( td_t *td )
{
  int rc;

  td->errnr = NO_ERROR;

  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    if( status_module_run( td, S5_ONLINE_STOP ) ) { // 0x81
      protokoll_stopp( td );
      return NO_ERROR;
    }
  }
  td->errnr = rc;
  return 1;
}

