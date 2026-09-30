#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#define  _S5LIB_C_
#include <as511_s5lib.h>

int as511_delete_module( td_t *td, unsigned char bst_typ, unsigned char bst_nr )
{
  int PrtStp_rc;
  unsigned char ch;

  td->errnr = 0;

  if( sigsetjmp(td->env, 1 ) == 0 ) {
    if( protokoll_start( td, S5_DELETE_MODULE ) ) {
      schreibe_daten_v2(td, bst_typ );
      schreibe_daten_v2(td, bst_nr );
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      PrtStp_rc = protokoll_stopp( td );

      if( PrtStp_rc == DC4 ) {
        PrtStp_rc = protokoll_stopp( td );
        td->errnr = MODULE_NOT_PRESENT;
        return 0;
      }
      return 1;
    }
  }
  return 0;
}
