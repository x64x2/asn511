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

int as511_write_ram( td_t *td, unsigned short adr, unsigned short laenge, unsigned char *ptr )
{
  unsigned char ch;
  unsigned int index = 0;

  td->errnr = 0;
  if( laenge > 512 )
    laenge = 512;

  if( sigsetjmp(td->env, 1) == 0 ) {
    if( protokoll_start( td, S5_WRITE_MEM ) ) {
      schreibe_daten_v2(td, HI(adr));
      schreibe_daten_v2(td, LO(adr));
      for( index = 0; index < laenge; index++ ) {
        schreibe_daten_v2(td, ptr[index]);
      }
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

int as511_write_ram32( td_t *td, unsigned long adr, unsigned long laenge, unsigned char *ptr )
{
  unsigned char ch;
  unsigned int index = 0;

  if( laenge > 1024 )
    laenge = 1024;

  if( sigsetjmp(td->env, 1) == 0 ) {
    if( protokoll_start( td, S5_WRITE_MEM ) ) {
      schreibe_daten_v2(td, HI(LHI(adr)));
      schreibe_daten_v2(td, LO(LHI(adr)));
      schreibe_daten_v2(td, HI(LLO(adr)));
      schreibe_daten_v2(td, LO(LLO(adr)));
      for( index = 0; index < laenge; index++ ) {
        schreibe_daten_v2(td, ptr[index]);
      }
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
