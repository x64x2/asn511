#include <setjmp.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#define __USE_XOPEN
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#define  _S5LIB_C_
#include <as511_s5lib.h>


bs_t *as511_read_module( td_t *td,  unsigned char btyp, unsigned char bnr )
{
  syspar_t  *sp  = NULL;
  bs_t *bst = NULL;

  unsigned char ch;
  unsigned int index = 0;

  td->errnr = 0;

  if( (sp = as511_read_system_parameter( td )) != NULL ) {
    if( sigsetjmp(td->env, 1 ) == 0 ) {
      if( protokoll_start( td, S5_READ_BST ) ) {
        schreibe_daten_v2(td, btyp);
        schreibe_daten_v2(td, bnr);
        schreibe_byte_v2(td, DLE);
        schreibe_byte_v2(td, EOT);
        lese_byte_v2(td, &ch, DLE, 1);
        lese_byte_v2(td, &ch, ACK, 1);
        lese_byte_v2(td, &ch, STX, 1);
        schreibe_byte_v2(td,DLE);
        schreibe_byte_v2(td,ACK);

        index =  as511_read_data( td );

        schreibe_byte_v2(td,DLE);
        schreibe_byte_v2(td,ACK);

        index -= (1 + sizeof(bs_kopf_t));

        if (protokoll_stopp( td ) ) {
          if( index > 0 ) {
            bst = Malloc(sizeof(bs_t));
            memcpy(&bst->kopf, &td->mem[1], sizeof(bs_kopf_t));
#if __BYTE_ORDER == __LITTLE_ENDIAN
            swab(&bst->kopf.laenge,&bst->kopf.laenge,sizeof(short));
#endif
            bst->laenge = bst->kopf.laenge * sizeof(short);
            bst->ptr = Malloc(bst->laenge - sizeof(bs_kopf_t));
            memcpy(bst->ptr, &td->mem[1 + sizeof(bs_kopf_t)], bst->laenge - sizeof(bs_kopf_t));
            as511_read_system_parameter_free( td, sp );
            return bst;
          }
        }
      }
    }

    as511_module_mem_free( td, bst );
    as511_read_system_parameter_free( td, sp );
  }
  return NULL;
}

void __inline__ as511_read_module_free( td_t * td, bs_t *bst )
{
  if( td ) {
    if( bst && bst->ptr ) {
      Free(bst->ptr);
      Free(bst);
    }
  }
}
