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

bal_t  *as511_read_module_addr_list( td_t *td, unsigned char bst_typ )
{
  bal_t *bal = NULL;
  syspar_t *sp;
  word_t l;

  unsigned char ch;
  unsigned int index = 0;

  td->errnr = 0;

  if( (sp = as511_read_system_parameter( td )) != NULL ) {
    if((l = as511_get_bst_addr_size( td, sp, bst_typ )) > 0 ) {
      if( sigsetjmp(td->env, 1) == 0 ) {
        if( protokoll_start( td, S5_READ_BST_ADDR_LIST ) ) {
          schreibe_daten_v2(td, bst_typ);
          schreibe_byte_v2(td, DLE);
          schreibe_byte_v2(td, EOT);
          lese_byte_v2(td, &ch, DLE, 1);
          lese_byte_v2(td, &ch, ACK, 1);
          lese_byte_v2(td, &ch, STX, 1);
          schreibe_byte_v2(td,DLE);
          schreibe_byte_v2(td,ACK);

          index = as511_read_data( td );

          schreibe_byte_v2(td,DLE);       //  PG 0x10   DLE
          schreibe_byte_v2(td,ACK);       //  PG 0x06   ACK

          if (protokoll_stopp( td ) ) {
            index--;
            bal = Malloc( sizeof( bal_t ) );
            bal->ptr = Malloc( l );

            memcpy(bal->ptr, &td->mem[1], l );
            bal->laenge = l;
#if __BYTE_ORDER == __LITTLE_ENDIAN
            swab(bal->ptr, bal->ptr, l );
#endif
            as511_read_system_parameter_free( td, sp );
            return bal;
          }
        }
      }
    }
  }
  as511_read_system_parameter_free( td, sp );
  return NULL;
}

void __inline__ as511_read_module_addr_list_free( td_t *td, bal_t *bal )
{
  if( td != NULL && bal != NULL ) {
    if( bal->ptr != NULL ) {
      Free(bal->ptr);
    }
    Free(bal);
  }
}
