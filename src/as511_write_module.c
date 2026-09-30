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

int as511_write_module(td_t *td,  bs_t *bst)
{
  syspar_t  *sp  = NULL; 
  unsigned char ch;
  unsigned int index = 0;

  unsigned char *ptr;
  unsigned char b;
  unsigned short bstlaenge;

  td->errnr = 0;

#if 1
  b = S5_WRITE_DB;
#else;
#endif
  if( (sp = as511_read_system_parameter( td )) != NULL ) {
    if( sigsetjmp(td->env, 1 ) == 0 ) {
      if( protokoll_start( td, b ) ) {
        schreibe_daten_v2(td, UCHAR(bst->kopf.baustein_typ.btyp));
        schreibe_daten_v2(td, UCHAR(bst->kopf.baustein_nummer));
        schreibe_daten_v2(td, HI(bst->kopf.laenge));
        schreibe_daten_v2(td, LO(bst->kopf.laenge));
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ETX);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ACK, 1);
        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td,DLE);
        schreibe_byte_v2(td,ACK);
        lese_byte_v2(td, &ch, HT, 1);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ETX, 1);
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, ACK);
        schreibe_byte_v2(td, STX);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ACK, 1);
        schreibe_byte_v2(td, NUL);

        bstlaenge = USHORT(bst->laenge);
#if __BYTE_ORDER == __LITTLE_ENDIAN
        swab(&bst->kopf.laenge,&bst->kopf.laenge,sizeof(short));
#endif
        ptr = (unsigned char *) &bst->kopf;
        for( index = 0; index < sizeof(bs_kopf_t); index++ )  {
          schreibe_daten_v2(td, ptr[index]);
        }
        for( index = 0; index < (bstlaenge) - sizeof(bs_kopf_t); index++ ) {
          schreibe_daten_v2(td, bst->ptr[index]);
        }
        schreibe_byte_v2(td,DLE);     
        schreibe_byte_v2(td,EOT);      
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ACK, 1);
        protokoll_stopp( td );
        as511_read_system_parameter_free( td, sp );
        return 1;
      }
    }
    as511_read_system_parameter_free( td, sp );
  }
  return 0;
}
