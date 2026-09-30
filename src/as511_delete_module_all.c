#include <setjmp.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#define  _S5LIB_C_
#include <as511_s5lib.h>

int as511_delete_module_all( td_t *td )
{
  int rc, PrtStart_rc;
  unsigned char ch;

  td->errnr = 0;

  if( (rc = sigsetjmp(td->env, 1 )) == 0 ) {
    if( (PrtStart_rc = protokoll_start( td, S5_DELETE_MODULE_ALL )) ) {
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      protokoll_stopp( td );
      if( PrtStart_rc == CR ) {
        td->errnr = ERROR_AG_RUNING;
        return 0;
      }
      return 1;
    }
  }
  return rc;
}
