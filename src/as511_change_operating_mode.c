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


int as511_change_operating_mode( td_t *td, unsigned char mode )
{
  int rc;
  unsigned char ch;

  switch( mode )
  {
    case S5_CH_OP_MODE_STOP:
    case S5_CH_OP_MODE_RESTART:
    case S5_CH_OP_MODE_REBOOT:
      if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
        if( protokoll_start( td, S5_CH_OP_MODE ) ) {
          schreibe_byte_v2(td, mode);
          schreibe_byte_v2(td, DLE);
          schreibe_byte_v2(td, EOT);
          lese_byte_v2(td, &ch, DLE, 1);
          lese_byte_v2(td, &ch, ACK, 1);
          if( mode == S5_CH_OP_MODE_RESTART || mode == S5_CH_OP_MODE_REBOOT ) {
            lese_byte_v2(td, &ch, STX, 1);
            schreibe_byte_v2(td, DLE);
            schreibe_byte_v2(td, ACK);
            lese_byte_v2(td, &ch, 0, 0);                       
            lese_byte_v2(td, &ch, DLE, 1);
            lese_byte_v2(td, &ch, ETX, 1);
            schreibe_byte_v2(td, DLE);
            schreibe_byte_v2(td, ACK);
          }
          protokoll_stopp( td );
          td->errnr = 0;
          return 1;
        }
      }
      break;
    default:
      td->errnr = BAD_PARAMETER;
      return 0;
  }
  td->errnr = rc;
  return 0;
}
