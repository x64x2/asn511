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

int as511_ag_stop( td_t *td )
{
  syspar_t  *sp  = NULL; 
  sps_ram_t *tmp = NULL; 

  if( (sp = as511_read_system_parameter( td )) != NULL ) {
    if( (tmp = as511_read_ram( td, USHORT (sp->sp.AddrSystemDaten + 12), 8 )) != NULL ) {
      tmp->ptr[0] |= SD_STOZUS;
      tmp->ptr[1] |= SD_AF; 
      as511_write_ram( td, USHORT (sp->sp.AddrSystemDaten + 12), 2, tmp->ptr );
      as511_read_ram_free( td, tmp );
    }
    as511_read_system_parameter_free( td, sp );
  }
  return 1;
}
