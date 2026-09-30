#ifndef _USTACK_H
#define _USTACK_H

#include "as511_s5lib.h"
struct ustack
{
  unsigned char *ptr;
  unsigned int   laenge;
};
typedef struct ustack ustack_t;

bool get_ustack_status_bit( ustack_t *m, int byteno, int bitno );

#if 0
#define BSTSCH((m))  get_status_bit(m,1,5)
#define SCHTAE((m))  get_status_bit(m,1,4)
#define ADRBAU((m))  get_status_bit(m,1,3)
#define SPABBR((m))  get_status_bit(m,1,2)

#define CADA((m))    get_status_bit(m,2,7)
#define CEDA((m))    get_status_bit(m,2,6)
#define REMAN((m))   get_status_bit(m,2,5)
#endif

void __inline__  as511_read_ustack_free(td_t *td, ustack_t *u );
ustack_t *as511_read_ustack(td_t * td );

#endif
