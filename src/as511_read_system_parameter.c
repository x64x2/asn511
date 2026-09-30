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

word_t as511_get_bst_addr_size( td_t *td, syspar_t *sp, byte_t bsttyp )
{
  word_t l;

  switch( bsttyp ) {
    case DB:
      l = sp->sp.Laenge_DB_liste;
      break;
    case SB:
      l = sp->sp.Laenge_SB_Liste;
      break;
    case PB:
      l = sp->sp.Laenge_PB_Liste;
      break;
    case FB:
      l = sp->sp.Laenge_FB_Liste;
      break;
    case OB:
      l = sp->sp.Laenge_OB_Liste;
      break;
    case DX:
      l = sp->sp.Laenge_DX_Liste;
      break;
    case FX:
      l = sp->sp.Laenge_FX_Liste;
      break;
    default:
      td->errnr = UNKNOWN_MODULE;
      l = 0;
      break;
  }
  return l;
}

syspar_t * as511_read_system_parameter( td_t *td )
{
  unsigned char ch;
  syspar_t *syspar = NULL;

  td->errnr = 0;

  if( sigsetjmp(td->env, 1) == 0 ) {
    if( protokoll_start( td, S5_READ_SYSPAR ) ) {
      schreibe_byte_v2(td, DLE);
      schreibe_byte_v2(td, EOT);
      lese_byte_v2(td , &ch, DLE, 1);
      lese_byte_v2(td, &ch, ACK, 1);
      lese_byte_v2(td, &ch, STX, 1);
      schreibe_byte_v2(td,DLE);
      schreibe_byte_v2(td,ACK);

      as511_read_data( td );
      schreibe_byte_v2(td,DLE);       
      schreibe_byte_v2(td,ACK);      

      if( protokoll_stopp( td ) ) {
#if __BYTE_ORDER == __LITTLE_ENDIAN
        swab(&td->mem[1],&td->mem[1],sizeof(sp_t) - 1);
#endif
        syspar = Malloc(sizeof(syspar_t));
        memcpy(&syspar->sp, &td->mem[1], sizeof(sp_t) );
        syspar->laenge = sizeof(sp_t);
        return syspar;
      }
    }
  }
  as511_read_system_parameter_free( td, syspar );

  return NULL;
}

void __inline__ as511_read_system_parameter_free( td_t * td, syspar_t * syspar )
{
  if( td && syspar )
    Free(syspar);
}
