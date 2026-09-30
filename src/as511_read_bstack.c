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

bstack_t *as511_read_bstack( td_t * td )
{
  byte_t ch;
  bstack_t *b = NULL;
  int PrtStart_rc;
  unsigned int index = 0;

  td->errnr = 0;
  if( sigsetjmp(td->env, 1) == 0 ) {
    if( (PrtStart_rc = protokoll_start( td, S5_READ_BSTACK )) ) {
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);

      if( PrtStart_rc != CR ) {
        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td,DLE);
        schreibe_byte_v2(td,ACK);

        index =  as511_read_data( td );

        schreibe_byte_v2(td,DLE);     
        schreibe_byte_v2(td,ACK);       

        if( --index ) {
          b = Malloc(sizeof(bstack_t));
          b->ptr = Malloc(index);
          b->laenge = index / sizeof(bstackfmt);
          memcpy(b->ptr, &td->mem[1], index);
#if __BYTE_ORDER == __LITTLE_ENDIAN
          swap(b->ptr,b->ptr, index);
#endif
        }
        else
          td->errnr = STACK_EMPTY;
      }
      else {
        td->errnr = ERROR_AG_RUNING;
      }

      if( protokoll_stopp( td ) ) {
        return b;
      }
    }
  }
  as511_read_bstack_free( td, b );
  return NULL;
}

void __inline__ as511_read_bstack_free( td_t *td, bstack_t *b )
{
  if( td && b ) {
    if( b->ptr ) {
      Free(b->ptr);
    }
    Free(b);
  }
}
