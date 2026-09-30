#include <setjmp.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <as511_s5lib.h>

int as511_compress_ram( td_t *td )
{
  unsigned char ch;

  td->errnr = 0;

  if( sigsetjmp(td->env, 1 ) == 0 ) {
    if( protokoll_start( td, S5_KOMPR_RAM ) ) {
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      protokoll_stopp( td );
      return 1;
    }
  }
  return 0;
}
