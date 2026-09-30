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

sps_ram_t * as511_read_ram( td_t *td, unsigned short adr, unsigned short laenge )
{
  sps_ram_t *tmp = NULL;
  unsigned char ch;
  unsigned int index = 0;

  td->errnr = 0;
  if( laenge > 512 )
    laenge = 512;

  if( sigsetjmp(td->env, 1) == 0 ) {
    if( protokoll_start( td, S5_READ_MEM ) ) {
      schreibe_daten_v2(td, HI(adr));
      schreibe_daten_v2(td, LO(adr));
      schreibe_daten_v2(td, HI(adr+laenge-1));
      schreibe_daten_v2(td, LO(adr+laenge-1));
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td,DLE);
      schreibe_byte_v2(td,ACK);
      index = as511_read_data( td );
      schreibe_byte_v2(td,DLE);       
      schreibe_byte_v2(td,ACK);       

      index -= 5; 

      if (protokoll_stopp( td ) ) {
        tmp = Malloc(sizeof(sps_ram_t));
        tmp->ptr = Malloc(index);
        tmp->laenge = index;
        memcpy(tmp->ptr, &td->mem[5], index);
        return tmp;
      }
    }
  }
  as511_read_ram_free( td, tmp );
  return NULL;
}

sps_ram_t * as511_read_ram32( td_t *td, unsigned long adr, unsigned long laenge )
{
  sps_ram_t *tmp = NULL;
  unsigned char ch;
  unsigned int index = 0;

  /* Zur Zeit werden Maximal 512 byte gelesen.
   */
  if( laenge > 512 )
    laenge = 512;

  if( sigsetjmp(td->env, 1) == 0 ) {
    if( protokoll_start( td, S5_READ_MEM ) ) {
      schreibe_daten_v2(td, HI(LHI(adr)));
      schreibe_daten_v2(td, LO(LHI(adr)));
      schreibe_daten_v2(td, HI(LLO(adr)));
      schreibe_daten_v2(td, LO(LLO(adr)));
      schreibe_daten_v2(td, HI(LHI(adr+laenge-1)));
      schreibe_daten_v2(td, LO(LHI(adr+laenge-1)));
      schreibe_daten_v2(td, HI(LLO(adr+laenge-1)));
      schreibe_daten_v2(td, LO(LLO(adr+laenge-1)));
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td,DLE);
      schreibe_byte_v2(td,ACK);
      index = as511_read_data( td );
      schreibe_byte_v2(td,DLE);
      schreibe_byte_v2(td,ACK);

      index -= 9; 

      if (protokoll_stopp( td ) ) {
        tmp = Malloc(sizeof(sps_ram_t));
        tmp->ptr = Malloc(index);
        tmp->laenge = index;
        memcpy(tmp->ptr, &td->mem[9], index);
        return tmp;
      }
    }
  }
  as511_read_ram_free( td, tmp );
  return NULL;
}

void __inline__ as511_read_ram_free( td_t *td, sps_ram_t *sr )
{
  if( td != NULL && sr != NULL ) {
    if( sr->ptr ) {
      Free(sr->ptr);
    }
    Free(sr);
  }
}
