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

/*
  Die Angaben sind beruhen teilweise auf vermutungen,
  da die entsprechende Hardeware nicht vorhanden ist.

  AG_100U CPU_090   "CPU 090"  ?
  AG_100U CPU_095   "CPU 095"  ?
  AG_100U CPU_100   "CPU 100"  ?
  AG_100U CPU_102   "CPU 102"  ?
  AG_100U CPU_103   "CPU 103"  Getestet Adresse 0XEA50
                               im Speicher der CPU (OK)

  AG_115U CPU_941   "CPU 941"  ?
  AG_115U CPU_942   "CPU 942"  ?
  AG_115U CPU_943   "CPU 943"  ?
  AG_115U CPU_944   "CPU 944"  ?
  AG_115U CPU_945   "CPU 945"  ?

  AG_135U CPU_921
  AG_135U CPU_922
  AG_135U CPU_928
  AG_135U CPU_928B  CPU Kennung 1 0x02 Getestet (OK)
                    CPU Kennung 2 0xB7
  AG_155U CPU_946   ?
  AG_155U CPU_947   ?
  AG_155U CPU_948   CPU Kennung 1 ?
                    CPU Kennung 2 0x82
  AG_155U CPU_948R  CPU Kennung 1 ?
                    CPU Kennung 2 0x83
*/

//
// CPU Typ und AG TYP ermitteln
// Wird für diverse Programmfunktionen benötigt.
// z.B für die ermittlung der Speicherorganisation Byte/Wort
//

#define AG_CPU100  "CPU 100"
#define AG_CPU102  "CPU 102"
#define AG_CPU103  "CPU 103"

#define AG_CPU941  "CPU 941"
#define AG_CPU942  "CPU 942"
#define AG_CPU943  "CPU 943"
#define AG_CPU944  "CPU 944"
#define AG_CPU945  "CPU 945"

#define AG_CPU921  "CPU 921"
#define AG_CPU922  "CPU 922"
#define AG_CPU928  "CPU 928"
#define AG_CPU928B "CPU 928B"

#define AG_CPU948  "CPU 948"
#define AG_CPU948R "CPU 948R"


ag_t ag100u[] = {
  { CPU100,AG100U,AG_CPU100 },
  { CPU102,AG100U,AG_CPU102 },
  { CPU103,AG100U,AG_CPU103 },
  { 0,0,NULL}
};

ag_t ag115u[] = {
  { CPU941,AG115U,AG_CPU941 },
  { CPU942,AG115U,AG_CPU942 },
  { CPU943,AG115U,AG_CPU943 },
  { CPU944,AG115U,AG_CPU944 },
  { CPU945,AG115U,AG_CPU945 },
  { 0,0,NULL}
};

ag_t ag135u[] = {
  { CPU921, AG135U,AG_CPU921  },
  { CPU922, AG135U,AG_CPU922  },
  { CPU928, AG135U,AG_CPU928  },
  { CPU928B,AG135U,AG_CPU928B },
  { 0,0,NULL}
};

ag_t ag155u[] = {
  { CPU948, AG155U,AG_CPU948  },
  { CPU948R,AG155U,AG_CPU948R },
  { 0,0,NULL}
};

ag_t *as511_get_ag_typ( td_t *td, syspar_t *sp )
{
  sps_ram_t *r;
  char      *cpu;
  int        i;
  ag_t      *ag = NULL;

  td->errnr = 0;

  switch( (sp->sp.CPU_Kennung & 0x03) ) {

    case 0x00: 

      if((r = as511_read_ram( td, USHORT (sp->sp.AddrSystemDaten +80), 12 )) != NULL ) {

        cpu = Malloc(r->laenge + 1 );
        memcpy(cpu, r->ptr, 12);
        cpu[7] = '\0';

        switch( (sp->sp.CPU_Kennung2 & 0x0F) ) {

            case 0x01:  
              for( i = 0; ag100u[i].cpu ; i++ ) {
                if(strncmp(cpu, ag100u[i].cpu, strlen(cpu) ) == 0 )
                  ag = &ag100u[i];
              }
              break;

            case 0x02:  
              ag = NULL;
              break;

            case 0x03:  
              ag = NULL;
              break;

            case 0x04:   
              for( i = 0; ag115u[i].cpu ; i++ ) {
                if(strncmp(cpu, ag115u[i].cpu, strlen(cpu) ) == 0 )
                  ag = &ag115u[i];
              }
              break;

            default:
              ag = NULL;
              break;

        }
        as511_read_ram_free( td, r );
        Free(cpu);
      }
      break;

    case 0x02: 

        switch( (sp->sp.CPU_Kennung2 & 0x0F) ) {

          case 0x07:  

              switch( ((sp->sp.CPU_Kennung2 & 0xF0) ) ) {

                case 0xB0: 
                  ag = &ag135u[3];
                  break;

                case 0x10: 
                  ag = &ag135u[1];
                  break;

                default:
                  ag = NULL;
                  break;

              }
              break;

          case 0x08:  

              switch( ((sp->sp.CPU_Kennung2 & 0xF0) ) ) {

                case 0x30: 
                  ag = &ag155u[1];
                  break;

                case 0x20: 
                  ag = &ag155u[0];
                  break;

                default:
                  ag = NULL;
                  break;

              }
              break;

          default:
            ag = NULL;
            break;

        }
        break;

    default:
      ag = NULL;
      break;

  }
  return ag;
}
