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

ustack_t *as511_read_ustack( td_t * td )
{
  byte_t ch;
  ustack_t *u = NULL;
  int PrtStart_rc;
  unsigned int index = 0;

  td->errnr = 0;
  if( sigsetjmp(td->env, 1 ) == 0 ) {
    if( (PrtStart_rc = protokoll_start( td, S5_READ_USTACK )) ) {
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);

      if( PrtStart_rc != CR ) {
        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td,DLE);
        schreibe_byte_v2(td,ACK);

        index = as511_read_data( td );

        schreibe_byte_v2(td,DLE);
        schreibe_byte_v2(td,ACK);

        if( --index ) {
          u = Malloc(sizeof(ustack_t));
          u->ptr = Malloc(index);
          u->laenge = index;
          memcpy(u->ptr, &td->mem[1], index);
        }
        else
          td->errnr = STACK_EMPTY;
      }
      else {
        td->errnr = ERROR_AG_RUNING;
      }

      if( protokoll_stopp( td ) ) {
        return u;
      }
    }
  }
  as511_read_ustack_free( td, u );
  return NULL;
}

void __inline__ as511_read_ustack_free( td_t *td, ustack_t *u )
{
  if( td && u ) {
    if( u->ptr ) {
      Free(u->ptr);
    }
    Free(u);
  }
}

