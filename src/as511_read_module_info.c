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

modinfo_t *as511_read_module_info( td_t *td, unsigned char bst_typ, unsigned char bst_nr )
{
  modinfo_t *bh = NULL;
  unsigned char ch;

  td->errnr = 0;

  if( sigsetjmp(td->env, 1) == 0 ) {
    if( protokoll_start( td, S5_READ_BOOKMARKER ) ) {
      bh = Malloc(sizeof(buchhalter_t));
      schreibe_daten_v2(td, bst_typ);
      schreibe_daten_v2(td, bst_nr);
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td, &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td,DLE);
      schreibe_byte_v2(td,ACK);

      as511_read_data( td );

      schreibe_byte_v2(td,DLE);       
      schreibe_byte_v2(td,ACK);       

      if (protokoll_stopp( td ) ) {
        memcpy(bh, &td->mem[1], sizeof(modinfo_t));
#if __BYTE_ORDER == __LITTLE_ENDIAN
        swab(&bh->ram_adresse, &bh->ram_adresse, sizeof(unsigned short));
        swab(&bh->laenge, &bh->laenge, sizeof(unsigned short));
#endif
        return bh;
      }
    }
  }
  as511_read_module_info_free( td, bh );
  return NULL;
}

void __inline__ as511_read_module_info_free( td_t * td, modinfo_t * mi )
{
  if( td && mi )
    Free(mi);
}
