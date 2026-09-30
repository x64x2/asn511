#include <setjmp.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <errno.h>
#define  _S5LIB_C_
#include "as511_s5lib.h"

#define DEBUG(x,y)  fprintf(td->debug_handle,(x),(y));

int lese_byte_v2( td_t *td, unsigned char *ch, unsigned char test_ch, int test_enable )
{
  int rc;
  struct pollfd  pfd;

  pfd.fd = td->fd;
  pfd.events = POLLIN;
  errno = 0;

  if( (rc = poll(&pfd, 1, td->timeout)) > 0 ) {
    read(td->fd,ch,1);

    if( td->debug_level >= DEBUG_LEVEL_AS511_ALL ) {
      DEBUG("\tAG -> PG %02X\n", *ch );
    }

    if( test_enable && *ch != test_ch ) {
      if( td->debug_level >= DEBUG_LEVEL_AS511 ) {
        fprintf(td->debug_handle,"Zeichen %02X anstatt %02X gelesen\n", *ch, test_ch );
      }
      td->errnr = CHAR_UNKNOWN;
      siglongjmp(td->env, CHAR_UNKNOWN );
    }
  }
  else {
    if( rc == 0 ) {
      if( td->debug_level >= DEBUG_LEVEL_AS511 ) {
        fprintf(td->debug_handle,"SPS Timeout: lese_byte_v2 %04X\n", SPS_TIMEOUT );
      }
      td->errnr = SPS_TIMEOUT;
      siglongjmp(td->env, SPS_TIMEOUT );
    }
    else {
      if( td->debug_level >= DEBUG_LEVEL_SYSTEM ) {
        fprintf(td->debug_handle,"Fehler in poll in funktion lese_byte_v2\n");
      }
      siglongjmp(td->env, rc);
    }
  }
  return rc;
}

int schreibe_byte_v2( td_t *td, unsigned char ch )
{
  int rc;
  struct pollfd pfd;

  pfd.fd = td->fd;
  pfd.events = POLLOUT;

  errno = 0;
  if( (rc = poll(&pfd, 1, td->timeout)) > 0 ) {
    write(td->fd,&ch,1);
    if( td->debug_level >= DEBUG_LEVEL_AS511_ALL ) {
      DEBUG("PG -> AG %02X\n", ch );
    }
  }
  else {
    if( rc == 0 ) {
      if( td->debug_level >= DEBUG_LEVEL_AS511 ) {
        fprintf(td->debug_handle,"SPS Timeout: schreibe_byte_v2\n");
      }
      td->errnr = SPS_TIMEOUT;
      siglongjmp(td->env, SPS_TIMEOUT);
    }
    else {
      if( td->debug_level >= DEBUG_LEVEL_SYSTEM ) {
        fprintf(td->debug_handle,"Fehler in poll in funktion schreibe_byte_v2\n");
      }
      siglongjmp(td->env, rc);
    }
  }
  return rc;
}

int schreibe_daten_v2( td_t *td, unsigned char ch )
{
  int rc;

  rc = schreibe_byte_v2( td, ch );
  if( rc == 1 && ch == 0x10 ) 
    rc = schreibe_byte_v2( td, ch );

  return rc;
}

td_t *open_tty( char *name )
{
  td_t  *td = Malloc(sizeof(td_t));

  td->timeout = TIMEOUT;

  if( (td->fd = open(name,O_RDWR|O_NONBLOCK)) > 0 ) 
  {
    fcntl(td->fd ,F_SETFL,fcntl(td->fd,F_GETFL,0) & ~O_NONBLOCK); 
    if( isatty(td->fd)) 
    {
      if( tcgetattr(td->fd, &td->term2 ) == 0 ) 
      {

        td->debug_level = 0;
        td->debug_handle = stderr;

        td->mem      = Malloc(MEM_SIZE);
        td->mem_size = MEM_SIZE;

        memcpy(&td->term1, &td->term2, sizeof(struct termios));

        cfsetispeed(&td->term1,B9600);
        cfsetospeed(&td->term1,B9600); 
        td->term1.c_lflag &= ~(ISIG | ECHO | ICANON | IEXTEN);

        td->term1.c_iflag &=
            ~(BRKINT | IGNBRK | IGNCR  | ICRNL  | INPCK  | ISTRIP |
              PARMRK | IXON );

        td->term1.c_cflag &= ~(CSIZE|PARODD);      
        td->term1.c_cflag |=  (CS8|PARENB|CSTOPB); 
        td->term1.c_oflag &= ~(OPOST|ONLCR);      
        td->term1.c_cc[VMIN]  = 1;  
        td->term1.c_cc[VTIME] = 0; 
        td->term1.c_cc[VEOF]  = 0;

        if( tcsetattr(td->fd, TCSAFLUSH, &td->term1) == 0 );
          return td;
      }
    }
  }

  tcsetattr(td->fd,TCSAFLUSH,&td->term2);
  if( td->fd > 0 )
    close( td->fd );
  Free(td);

  return NULL;
}

int close_tty( td_t * td )
{
  tcsetattr(td->fd,TCSAFLUSH,&td->term2); 
  close(td->fd);               
  free( td->mem );
  free( td );
  return 1;
}

int protokoll_start( td_t *td, unsigned char bef )
{
  unsigned char ch;
  int rc;

  schreibe_byte_v2(td, STX);
  lese_byte_v2(td, &ch, DLE, 1);
  lese_byte_v2(td, &ch, ACK, 1);
  schreibe_daten_v2(td, bef);    
  lese_byte_v2(td, &ch, STX, 1);
  schreibe_byte_v2(td, DLE);
  schreibe_byte_v2(td, ACK);
  lese_byte_v2(td, &ch, 0, 0);
  rc = (int) ch;
  if( ch == CR ) {
    lese_byte_v2(td, &ch, STX, 1);
  }
  lese_byte_v2(td, &ch, DLE, 1);
  lese_byte_v2(td, &ch, ETX, 1);
  schreibe_byte_v2(td, DLE);
  schreibe_byte_v2(td, ACK);
  return rc;
}

int protokoll_stopp( td_t *td )
{
  unsigned char ch;
  int rc = 0;

  lese_byte_v2(td, &ch, STX, 1);
  schreibe_byte_v2(td,DLE);
  schreibe_byte_v2(td,ACK);
  lese_byte_v2(td, &ch, 0, 0);

  rc = (int) ch;
  switch( rc ) {
    case DLE: 
      lese_byte_v2(td, &ch, 0, 0);
      if( ch == ETX ) {
        schreibe_byte_v2(td,DLE);
        schreibe_byte_v2(td,ACK);
        return rc;
      }
      break;

    case DC1: 
      break;
    case DC2: 
      break;

    case DC4: 
      break;

    default:
      if( td->debug_level > DEBUG_LEVEL_AS511 ) {
        fprintf(td->debug_handle,
                "In Funktion protokoll_stopp:\n" \
                "Unerwartetes Zeichen %02X vom AG\n", ch );
      }
      siglongjmp(td->env, CHAR_UNKNOWN );
      break;
  }

  lese_byte_v2(td, &ch, DLE, 1);
  lese_byte_v2(td, &ch, 0, 0);
  schreibe_byte_v2(td,DLE);
  schreibe_byte_v2(td,ACK);

  return rc;
}

void __inline__ as511_module_mem_free( td_t *td, bs_t *bst )
{
  if( td && bst ) {
    if( bst->ptr ) {
      Free(bst->ptr);
    }
    Free( bst );
  }
}

int as511_set_bst_data( bs_t *bst, byte_t btyp, byte_t bnr, word_t code_size, byte_t *code )
{
  bst->laenge = code_size + sizeof(bs_kopf_t); 
  bst->kopf.baustein_sync1 = 0x70;
  bst->kopf.baustein_sync2 = 0x70;
  bst->kopf.baustein_nummer = bnr; 
  bst->kopf.baustein_typ.btyp = btyp; 
  bst->kopf.baustein_typ.bok = (unsigned)0;
  bst->kopf.pg_kennung = 0x80;
  bst->kopf.bib_nummer1 = 0;
  bst->kopf.bib_nummer2 = 0;
  bst->kopf.bib_nummer3 = 0;
  bst->kopf.laenge = (unsigned short)((code_size + sizeof(bs_kopf_t)) / sizeof(word_t));
  bst->ptr = code;
  return 1;
}

int as511_read_data( td_t *td )
{
  int index = 0;
  unsigned char ch = 0;
  int DLEret = 0;

  while( 1 ) {
    lese_byte_v2(td,&ch,0,0 );
    if( ch == ETX && DLEret )
      break;
    if( (DLEret = (ch == DLE && !DLEret )) )
      continue;
    td->mem[index++] = ch;
  }
  return index;
}

int copy_module_data( td_t *td, int index,  smd_u *smd )
{
  if( td && smd ) {
    switch( smd->t.type ) {
      case DEBUG_MODULE_PAE:
      case DEBUG_MODULE_PAA:
      case DEBUG_MODULE_MERKER:
      case DEBUG_MODULE_NOPAR:
        memcpy(&DEBUG_MODULE_AG_ADDR(smd), &td->mem[index], 4 );
        index += 4;
        break;
      case DEBUG_MODULE_ZAEHLER:
      case DEBUG_MODULE_DATEN:
        memcpy(&DEBUG_MODULE_AG_ADDR(smd), &td->mem[index], 6 );
        index += 6;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        swab(&DEBUG_MODULE_WORD_VALUE(smd),&DEBUG_MODULE_WORD_VALUE(smd),sizeof(short));
#endif
        break;
      case DEBUG_MODULE_LOAD:
        memcpy(&DEBUG_MODULE_AG_ADDR(smd), &td->mem[index], 8 );
        index += 8;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        swab(&DEBUG_MODULE_AKKU1(smd),&DEBUG_MODULE_AKKU1(smd),sizeof(short));
        swab(&DEBUG_MODULE_AKKU2(smd),&DEBUG_MODULE_AKKU2(smd),sizeof(short));
#endif
        break;
      case DEBUG_MODULE_LOAD_LARGE:
        memcpy(&DEBUG_MODULE_AG_ADDR(smd), &td->mem[index], 12 );
        index += 12;
#if __BYTE_ORDER == __LITTLE_ENDIAN
        swab(&DEBUG_MODULE_AKKU1L(smd),&DEBUG_MODULE_AKKU1L(smd),sizeof(int));
        swab(&DEBUG_MODULE_AKKU2L(smd),&DEBUG_MODULE_AKKU2L(smd),sizeof(int));
#endif
        break;
    }
#if __BYTE_ORDER == __LITTLE_ENDIAN
    swab(&DEBUG_MODULE_AG_ADDR(smd),&DEBUG_MODULE_AG_ADDR(smd),sizeof(short));
#endif
  }
  return index;
}
