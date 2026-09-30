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

raminfo_t *as511_read_ram_info( td_t *td )
{
  syspar_t  *sp;
  int index = 0;
  unsigned char ch;
  raminfo_t *tmp = NULL;

  td->errnr = 0;

  if( (sp = as511_read_system_parameter(td)) != NULL ) {
    if( sigsetjmp(td->env, 1) == 0 ) {
      if( protokoll_start( td, S5_READ_RAM_INFO ) ) {
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

        if( index > 0 ) {
          index -= 1; 
        }

        if (protokoll_stopp( td ) ) {
          tmp = Malloc(sizeof(raminfo_t));
          memcpy(tmp, &td->mem[1], index);
#if __BYTE_ORDER == __LITTLE_ENDIAN
          swab(tmp, tmp, index );
#endif
          tmp->end_ram = sp->sp.AddrEndRam;
        }
      }
    }
    as511_read_system_parameter_free( td, sp );
  }
  return tmp;
}
