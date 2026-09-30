#include <unistd.h>
#include <semaphore.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <as511_s5lib.h>
#include <as511_ustack.h>
#include "main.h"

struct bstyp
{
  char *s;
  int   n;
} b[] = {
  { "DB", DB },
  { "SB", SB },
  { "FB", FB },
  { "PB", PB },
  { "OB", OB },
  { "FX", FX },
  { "DX", DX },
  { NULL, 0  },
};

int EnCompress;
int EnModRead;
int EnModWrite;
int EnRam;
int EnStatusVar;
int EnStatusBst;
int EnDelete;
int ReadSysPar;
int ReadBst;
int ReadBstack;
int ReadUstack;
int StepModule;
int AgRun;
int AgStop;
int ReadBstInfo;
int ReadAdrLst;
int   BstTrfFdAg;
int   BstTrfAgFd;
char *Mc5File;
char *BstTyp;
int   BstNr;
char *CtrlOutp;

int Start;
int Size;
int  debug;     
char *dev = "/dev/ttyS0"; 
int help;
int usage;

struct poptOption option[] = {
  { "run",                       0, POPT_ARG_NONE,   &AgRun,      0x0001, "AG Starten" },
  { "stop",                      0, POPT_ARG_NONE,   &AgStop,     0x0002, "AG Stoppen" },
  { "compress-ag",             'c', POPT_ARG_NONE,   &EnCompress, 0x0003, "AG Speicher Komprimieren" },
  { "bst-transfer-hd-ag",      'F', POPT_ARG_NONE,   &BstTrfFdAg, 0x1002, "Transfer Baustein HD -> AG" },
  { "bst-transfer-ag-hd",      'A', POPT_ARG_NONE,   &BstTrfAgFd, 0x1003, "Transfer Baustein AG -> HD" },
  { "status-var",              'v', POPT_ARG_NONE,   &EnStatusVar,0x1009, "Status Variable" },
  { "status-bst",              'B', POPT_ARG_NONE,   &EnStatusBst,0x1010, "Status Baustein" },
  { "delete-module",           'e', POPT_ARG_NONE,   &EnDelete,   0x1011, "Bausteine Löschen" } ,
  { "read-bst-info",           'i', POPT_ARG_NONE,   &ReadBstInfo,0x1022, "Baustein Info Lesen" },
  { "read-memory",             'm', POPT_ARG_NONE,   &EnRam,      0x1023, "Lesen eines Speicherbereiches" },
  { "read-system-parameter",   'p', POPT_ARG_NONE,   &ReadSysPar, 0x1012, "Systemparameter Lesen" },
  { "read-baust-addr-list",    'b', POPT_ARG_NONE,   &ReadAdrLst, 0x1013, "Bausteinadressliste Lesen" },
  { "read-bstack",               0, POPT_ARG_NONE,   &ReadBstack, 0x1014, "Baustein Stack Lesen" },
  { "read-ustack",               0, POPT_ARG_NONE,   &ReadUstack, 0x1014, "Unterbrechungs Stack Lesen" },
  { "dev",                     'd', POPT_ARG_STRING, &dev,        0x2001, "Dateiname der SPS Schnittstelle","/dev/ttyS0"},
  { "start-mem-addr",          'a', POPT_ARG_INT,    &Start,      0x2002, "Startadresse des AG Speicherbereiches", "0xEA00" },
  { "size-mem-addr",           's', POPT_ARG_INT,    &Size,       0x2003, "Länge des zu Lesenden AG Speicherbereiches", "512" },
  { "mc5file",                 'f', POPT_ARG_STRING, &Mc5File,    0x2004, "Dateiname", "DATEI.MC5" },
  { "module-type",             't', POPT_ARG_STRING, &BstTyp,     0x2005, "Baustein typ", "DB, OB, SB, FB ..." },
  { "module-nummer",           'n', POPT_ARG_INT,    &BstNr,      0x2006, "Baustein Nummer", "0/1-255" },
  { "ctrl-output",             'o', POPT_ARG_STRING, &CtrlOutp,   0x2007, "Ausgangsbytes Ansteuern !!ACHTUNG VORSICHT!!", "Byteadresse,Wert,..." },
  { "step-module",             'S', POPT_ARG_NONE,   &StepModule, 0x1015, "Bearbeitungskontrolle" },
  { "debuglevel",              'D', POPT_ARG_INT,    &debug,      0x4001, "Debuglevel 0=Keine 30=Alle Ausgaben", "0 .. 30" },
  POPT_AUTOHELP
  { NULL, 0, 0, NULL, 0 }
};

unsigned char ob1[] = {
      0xA0,  
      0x00,
      0x30,   
      0x02,
      0x11,
      0x00,
      0x24,   
      0x00,
      0xF8,   
      0x00,
      0x98,  
      0x00,
      0xA0,  
      0x00,
      0x81,  
      0x00,
      0xFB,  
      0x00,
      0x80,   
      0x00,
      0xA1,  
      0x00,
      0x99,   
      0x00,
      0x65,   
      0x00
};

unsigned char db62[] = {
      0x12,
      0x34,
      0x56,
      0x78,
      0x12,
      0x34,
      0x56,
      0x78
};

#define LN(x) ((x) & 0x000F)         /* low  nibble */
#define HN(x) (((x) >> 4) & 0x000F)  /* high nibble */

int dumpdata( unsigned char *ch, unsigned long size )
{
  unsigned int i,j;
  unsigned char cp[17], c;

  for( i = j = 0; size-- ; j++ )
  {
    c = ch[j];

    cp[i] = UCHAR (isprint(c) ? c : '.');
    if( !(j % 16) )
      printf("%04d  ", (int)j);

    putc( HN(c) > 9 ? HN(c) + 'A' - 10 : HN(c) + '0', stdout);
    putc( LN(c) > 9 ? LN(c) + 'A' - 10 : LN(c) + '0', stdout);
    putc( ' ', stdout );

    if( !(++i % 16) )
    {
      cp[i] = 0;
      puts((char*)cp);
      i = 0;
    }
  }

  cp[i] = 0;
  c = UCHAR (16 - i);
  c *= 3;

  while( c-- )
    putc(' ',stdout);

  puts((char*)cp);
  return 0;
}

bs_t *lese_mc5( char *pfad )
{
  int h;        
  struct stat s; 
  bs_t *bst;    

  if( (h = open(pfad,O_RDONLY)) >= 0 ) {
    if( fstat( h, &s ) == 0 ) {
      if(S_ISREG(s.st_mode) ) {
        bst = Malloc(sizeof(bs_t));
        bst->ptr = Malloc(s.st_size-sizeof(bs_kopf_t));
        read( h, &bst->kopf, sizeof(bs_kopf_t) );
        read( h, bst->ptr, s.st_size - sizeof(bs_kopf_t));
        close(h);
        bst->laenge = s.st_size;
        return bst;
      }
    }
  }
  return NULL;
}

int schreibe_mc5( char *pfad, bs_t *bst )
{
  int h;
  int size = 0;

  if( (h = open( pfad, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR )) >= 0 ) {
    size += write(h, &bst->kopf, sizeof(bs_kopf_t));
    size += write(h, bst->ptr, bst->laenge - sizeof(bs_kopf_t));
    close(h);
  }
  return size == bst->laenge;
}

char * BstTypToString( u8 bsttyp )
{
  int i;

  for( i = 0; b[i].s; i++ ) {
    if( b[i].n == bsttyp ) {
      return b[i].s;
    }
  }
  return NULL;
}

int StringToBstTyp( char *bststring )
{
  int i;

  for( i = 0; b[i].s; i++ ) {
    if( strcasecmp(b[i].s,bststring) == 0 ) {
      return b[i].n;
    }
  }
  return 0;
}

struct termios term;

void SetTermBlockOff( int h )
{
  fcntl(h ,F_SETFL,fcntl(h,F_GETFL,0) | O_NONBLOCK);
}

void SetTermBlockOn( int h )
{
  fcntl(h ,F_SETFL,fcntl(h,F_GETFL,0) & ~O_NONBLOCK);
}

int SetTerm( void )
{
  int h;

  if( isatty(h = fileno(stdin)) ) {
    tcgetattr(h, &term);
    term.c_lflag &= ~(ICANON | ECHO );
    tcsetattr(h, TCSANOW, &term);
  }
  return 1;
}

int ReSetTerm( void )
{
  int h;

  if( isatty(h = fileno(stdin)) ) {
    tcgetattr(h, &term);
    term.c_lflag |= (ICANON | ECHO | ISIG);
    tcsetattr(h, TCSANOW, &term);
    fcntl(h ,F_SETFL,fcntl(h,F_GETFL,0) & ~O_NONBLOCK);
  }
  return 1;
}

int KbHit( void )
{
  int k = 0;
  int h;
  if( isatty(h = fileno(stdin)) ) {
    SetTerm() ;
    SetTermBlockOff(h);
    read(h,&k,1);
    SetTermBlockOn(h);
    ReSetTerm();
  }
  return k;
}

int GetKey( void )
{
  int k = 0;
  int h;
  if( isatty(h = fileno(stdin)) ) {
    SetTerm();
    read(h,&k,1);
    ReSetTerm();
  }
  return k;
}

void PrintSystemParameter( syspar_t *sp )
{
  printf("\n");
  printf("Adresse Eingangssignalformer. %04X\n",sp->sp.AddrESF);
  printf("Adresse Ausgangssignalformer. %04X\n",sp->sp.AddrASF);
  printf("pae_digital ................. %04X\n",sp->sp.AddrPAE_Digital);
  printf("paa_digital ................. %04X\n",sp->sp.AddrPAA_Digital);
  printf("merker ...................... %04X\n",sp->sp.AddrMerker);
  printf("zeiten ...................... %04X\n",sp->sp.AddrZeiten);
  printf("zaehler ..................... %04X\n",sp->sp.AddrZaehler);
  printf("systemdaten ................. %04X\n",sp->sp.AddrSystemDaten);
  printf("System Prog Ram ............. %04X\n",sp->sp.SystemProgRam);
  printf("Statuskennung ............... %02X\n",sp->sp.StatusKennung);
  printf("AG SW Version ............... %02X\n",sp->sp.AG_sw_version);
  printf("addr_end .................... %04X\n",sp->sp.AddrEndRam);
  printf("Länge DB Liste............... %d\n",sp->sp.Laenge_DB_liste);
  printf("Laenge SB Liste.............. %d\n",sp->sp.Laenge_SB_Liste);
  printf("Laenge PB Liste.............. %d\n",sp->sp.Laenge_PB_Liste);
  printf("Laenge FB Liste.............. %d\n",sp->sp.Laenge_FB_Liste);
  printf("Laenge OB Liste.............. %d\n",sp->sp.Laenge_OB_Liste);
  printf("Laenge TB/FX Liste........... %d\n",sp->sp.Laenge_FX_Liste);
  printf("Laenge DX Liste.............. %d\n",sp->sp.Laenge_DX_Liste);
  printf("Laenge_DB0_Liste............. %d\n",sp->sp.Laenge_DB0_Liste);
  printf("Steckplatzkenng ............. %02X\n",sp->sp.Steckplatzkenng);
  printf("BstKopfLaenge ............... %d\n",sp->sp.BstKopfLaenge);
  printf("CPU Kennung.................. %02X\n",sp->sp.CPU_Kennung);
  printf("CPU Kennung 2................ %02X\n",sp->sp.CPU_Kennung2);
  printf("Unbek_7 ..................... %02X\n",sp->sp.unbek_7);
  printf("Unbek_8 ..................... %04X\n",sp->sp.unbek_8);
  printf("Unbek_9 ..................... %04X\n",sp->sp.unbek_9);
  printf("Unbek_10 .................... %04X\n",sp->sp.unbek_10);
  printf("\n");
}

void PrintModuleInfo( modinfo_t *mi )
{
  printf("\n");
  printf("Ram Adresse   = %04X\n", mi->ram_adresse);
  printf("Sync 1        = %02X\n", mi->baustein_sync1);
  printf("Sync 2        = %02X\n", mi->baustein_sync2);
  printf("Baustein Nr.  = %02x\n", mi->baustein_nummer);
  printf("Baustein Typ  = %02X\n", mi->bst.btyp);
  printf("Baustein OK   = %02X\n", mi->bst.bok);
  printf("PG Kennung    = %02X\n", mi->pg_kennung);
  printf("BIB 1         = %02X\n", mi->bib_nummer1);
  printf("BIB 2         = %02X\n", mi->bib_nummer2);
  printf("BIB 3         = %02X\n", mi->bib_nummer3);
  printf("Bausteinlänge = %d\n",   mi->laenge);
  printf("\n");
}

int lese_system_parameter( td_t *td )
{
  syspar_t *sp;
  ag_t     *ag;

  printf("Systemparameter Lesen,\n");

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {
    if( (ag = as511_get_ag_typ(td, sp )) != NULL )
      printf("AG TYP = %s\n\n", ag->cpu);

    PrintSystemParameter( sp );
    as511_read_system_parameter_free( td, sp );
  }
  return 1;
}

int ag_run( td_t *td )
{
  syspar_t *sp;
  ag_t     *ag;

  printf("AG in RUN Schalten ? y/N,\n");

  if( GetKey() == 'y' ) {
    if( (sp = as511_read_system_parameter(td)) == NULL ) {
      printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
      return 0;
    }
    if( (ag = as511_get_ag_typ(td, sp )) != NULL ) {
      printf("AG wird gestartet\n");
      if( ag->ag_typ == AG100U ) {
        as511_ag_run(td);
      }
      else {
        if( ag->ag_typ == AG135U || ag->ag_typ == AG155U ) {
          as511_change_operating_mode( td, S5_CH_OP_MODE_RESTART );
        }
        else {
          printf("Es werden momentan nur die AG Typen"
                 "AG100U und AG135U unterstützt\n\n");
        }
      }
    }
    as511_read_system_parameter_free( td, sp );
  }
  return 1;
}

int ag_stop( td_t *td )
{
  syspar_t *sp;
  ag_t     *ag;
  int       rc = 0;

  printf("AG in STOP Schalten ? y/N,\n");

  if( GetKey() == 'y' ) {
    if( (sp = as511_read_system_parameter(td)) == NULL ) {
      printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
      return 0;
    }

    if( (ag = as511_get_ag_typ(td, sp )) != NULL ) {
      printf("AG wird gestoppt\n");
      if( ag->ag_typ == AG100U ) {
        as511_ag_stop(td);
        rc = 1;
      }
      else {
        if( ag->ag_typ == AG135U || ag->ag_typ == AG155U ) {
          as511_change_operating_mode( td, S5_CH_OP_MODE_STOP );
          rc = 1;
        }
        else {
          printf("Es werden momentan nur die AG Typen"
                 "AG100U und AG135U unterstützt\n\n");
        }
      }
    }
    as511_read_system_parameter_free( td, sp );
  }
  return rc;
}

int BstTransferAgFd( td_t *td, byte_t btyp, byte_t bnr, char *pfad )
{
  int    rc = 0;
  struct stat s;

  syspar_t *sp;
  bs_t     *bst;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return rc;
  }
  as511_read_system_parameter_free( td, sp );

  if( (bst = as511_read_module(td, btyp, bnr )) != NULL ) {
    if( stat( pfad, &s ) == 0 ) {
      printf("Datei %s schon vorhanden! Überschreiben ? y/N\n", pfad);
      if( GetKey() != 'y' ) {
        as511_module_mem_free( td, bst );
        return rc;
      }
    }
    else {
      rc = schreibe_mc5( pfad, bst );
    }
    as511_module_mem_free( td, bst );
  }
  return rc;
}

int BstTransferFdAg( td_t *td, char *pfad )
{
  int    rc = 0;
  int    wr = 0;
  byte_t    btyp;
  byte_t    bnr;
  syspar_t *sp  = NULL; 
  bs_t     *bst = NULL; 
  bal_t    *ba  = NULL;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return rc;
  }

  if( (bst = lese_mc5( pfad )) != NULL ) {
    if( SYNC1(bst) == 0x70 && SYNC2(bst) == 0x70 ) {
      btyp = UCHAR (BSTTYP(bst));
      bnr  = BSTNR(bst);
      if( (ba = as511_read_module_addr_list( td, btyp )) != NULL ) {
        if( ba->ptr[bnr] != 0 ) {
          printf("Baustein %s%d bereits im AG! Überschreiben ? y/N\n", BstTypToString((u8)btyp),bnr );
          if( GetKey() == 'y' ) {
            wr = 1;
          }
        }
        else {
          wr = 1;
        }
      }

      if( wr )
        as511_write_module(td, bst );
    }
  }

  as511_read_system_parameter_free( td, sp );
  as511_module_mem_free( td, bst );
  as511_read_module_addr_list_free( td, ba );
  return rc;
}

int read_module_info( td_t *td, unsigned char bsttyp, unsigned char bstnr )
{
  modinfo_t *mi;
  syspar_t  *sp;
  bal_t     *ba;
  bs_t      *bs;
  int i;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }

  if( (ba = as511_read_module_addr_list( td, bsttyp )) != NULL ) {
    for( i = 0; i < ba->laenge / 2; i++ ) {
      if( ba->ptr[i] ) {
        printf("Es ist Baustein %s%03d an Adresse %04X im SPS Speicher\n",
               BstTypToString((u8)bsttyp), i, ba->ptr[i]);
      }
    }

    if( ba->ptr[bstnr] != 0 ) {
      if( (mi = as511_read_module_info( td, bsttyp, bstnr )) != NULL ) {
        PrintModuleInfo(mi);
        as511_read_module_info_free( td, mi );
      }

      if( (bs = as511_read_module(td, bsttyp, bstnr )) != NULL ) {
        dumpdata(bs->ptr, bs->laenge - sizeof(bs_kopf_t));
        as511_read_module_free( td, bs );
      }
    }
    else {
      printf("Der Angegebene Baustein %s%03d ist nicht im AG\n",
             BstTypToString((u8)bsttyp), bstnr );
    }
    as511_read_system_parameter_free( td, sp );
    as511_read_module_addr_list_free( td, ba );
  }
  return 0;
}

int test_write_module( td_t *td, bs_t *bst )
{
  syspar_t  *sp;
  bal_t     *ba;
  bs_t      *bs;
  char       s[128];

  int i;
  int write_ok = 1;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {
    PrintSystemParameter( sp );
    as511_read_system_parameter_free( td, sp );
  }

  if( (ba = as511_read_module_addr_list( td, UCHAR(bst->kopf.baustein_typ.btyp) )) != NULL ) {
    for( i = 0; i < ba->laenge / 2; i++ ) {
      if( ba->ptr[i] ) {
        printf("Baustein %s %d an Adresse %04X im SPS Speicher\n",
               BstTypToString((u8)bst->kopf.baustein_typ.btyp), i, ba->ptr[i]);
      }
    }

    if( (ba->ptr[bst->kopf.baustein_nummer] != 0) ) {
      printf("!! Achtung Baustein %s%d im AG bereits vorhanden !! Überscheiben ? y/N\n",
             BstTypToString((u8)bst->kopf.baustein_typ.btyp), bst->kopf.baustein_nummer);
      if( GetKey() != 'y' ) {
        write_ok = 0;
      }
    }

    if( write_ok ) {
      if( ba->ptr[bst->kopf.baustein_nummer] != 0 ) { 
        if((bs = as511_read_module( td,
            UCHAR(bst->kopf.baustein_typ.btyp),
            bst->kopf.baustein_nummer )) != NULL ) {
              printf("Alten Baustein %s%d Sichern\n",
                     BstTypToString((u8)bst->kopf.baustein_typ.btyp),
                     bst->kopf.baustein_nummer);
              sprintf(s,"%s%d.mc5",
                      BstTypToString((u8)bst->kopf.baustein_typ.btyp),
                      bst->kopf.baustein_nummer);
              schreibe_mc5(s, bs );
              as511_module_mem_free( td, bs );
        }
        else {
          printf("%p\n", bs);
        }
      }
      as511_write_module(td, bst );
      as511_read_module_addr_list_free( td, ba );
    }
    else
      return 0;
  }
  return 1;
}

int read_module_addr_list( td_t *td, char *bsttyp )
{
  syspar_t  *sp;
  bal_t     *ba;
  unsigned char b;

  int i;
  int f = 0;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }

  if( (b = UCHAR (StringToBstTyp(bsttyp))) != '\0' ) {
    if( (ba = as511_read_module_addr_list( td, b )) != NULL ) {
      printf("Länge Bausteinaddressliste = %d Wörter\n",(int)(ba->laenge / 2) );
      for( i = 0; i < ba->laenge / 2; i++ ) {
        if( ba->ptr[i] ) {
          printf("Baustein %s %3d an Adresse %04X im SPS Speicher\n",
                 BstTypToString((u8)b), i, ba->ptr[i]);
          f++;
        }
      }
      printf("%d Bausteine des Typs %s im AG\n", f, BstTypToString((u8)b) );
      as511_read_module_addr_list_free( td, ba );
    }
  }
  else {
    printf("Fehler: Ungültiger Bausteintyp %s\n", bsttyp );
  }
  as511_read_system_parameter_free( td, sp );
  return 1;
}

int test_ram( td_t *td , dword_t start, dword_t size )
{
  syspar_t  *sp;
  ram_t     *ram;
  ag_t      *ag;

  unsigned long AddrData = start;
  unsigned long SizeData = size;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {

    if( SizeData == 0 ||  SizeData > 512)
      SizeData = 512;

    PrintSystemParameter( sp );
  }


  printf("Lese %lu bytes ab Adresse 0x%08lX\n",SizeData,AddrData);
  ag = as511_get_ag_typ( td, sp );

  if( ag && ag->ag_typ == AG155U ) {
    if( (ram = as511_read_ram32(td, AddrData, SizeData )) != NULL ) {
      dumpdata(ram->ptr, ram->laenge);
    }
  }
  else {
    if( (ram = as511_read_ram(td, (word_t)AddrData, (word_t)SizeData )) != NULL ) {
      dumpdata(ram->ptr, ram->laenge);
    }
  }
  as511_read_ram_free( td, ram );
  as511_read_system_parameter_free( td, sp );
  return 1;
}

int test_compress( td_t *td )
{
  syspar_t  *sp;
  raminfo_t *v;
  raminfo_t *n;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {
    PrintSystemParameter( sp );
    as511_read_system_parameter_free( td, sp );
  }

  if( (v = as511_read_ram_info( td )) != NULL ) {
    as511_compress_ram( td );
    if( (n = as511_read_ram_info( td )) != NULL ) {
      printf( "Ram info vor Komprimieren\n"
              "RAM start ...................%04X\n"
              "RAM beginn freier Bereich .. %04X\n"
              "RAM ende ................... %04X\n",
              v->start_ram,
              v->begin_free_ram,
              v->end_ram
            );

      printf("\nAG Speicher Komprimieren\n");

      printf( "Ram info nach Komprimieren\n"
              "RAM start ...................%04X\n"
              "RAM beginn freier Bereich .. %04X\n"
              "RAM ende ................... %04X\n",
              n->start_ram,
              n->begin_free_ram,
              n->end_ram
            );
      printf("Durch das Komprimieren wurde %d Byte Speicher frei\n",
             v->begin_free_ram - n->begin_free_ram );
      Free(v);
      Free(n);
    }
  }
  return 0;
}

int test_delete( td_t *td, unsigned char bsttyp, unsigned char bstnr )
{
  syspar_t  *sp;
  bal_t     *ba;

  int delete_ok = 1;
  int module_present = 1;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {
    PrintSystemParameter( sp );
    as511_read_system_parameter_free( td, sp );
  }

  if( (ba = as511_read_module_addr_list( td, bsttyp )) != NULL ) {
    if( (ba->ptr[bstnr] != 0) ) {
      printf("!! Achtung Baustein %s%d im AG vorhanden !! Löschen ? y/N\n",BstTypToString((u8)bsttyp),bstnr);
      if( GetKey() != 'y' ) {
        delete_ok = 0;
      }
    }
    else {
      printf("!! Achtung Baustein %s%d nicht im AG vorhanden\n",BstTypToString((u8)bsttyp),bstnr);
      module_present = 0;
    }

    if( delete_ok && module_present ) {
      as511_delete_module(td, bsttyp, bstnr );
    }
    as511_read_module_addr_list_free( td, ba );
  }
  return 1;
}

int test_status_var( td_t *td, char *varlist )
{
  char *runtimer[] = { "Steht ", "Laeuft" };

  int i;
  int bit;
  dl_t  *dl;
  svd_u *svd;
  syspar_t  *sp;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {
    PrintSystemParameter( sp );
  }

  printf("Status Variable Starten ? Y/n\n");
  printf("Beliebige Taste Stoppt die Funktion Status Var\n");

  if( GetKey() == 'n' ) {
    return 0;
  }

  as511_status_var_create( td );

  as511_status_var_insert_type( td, STATUS_VAR_DATEN, sp->sp.AddrZeiten, NULL, NULL );
  as511_status_var_insert_type( td, STATUS_VAR_DATEN, USHORT(sp->sp.AddrZeiten + 2U), NULL, NULL );
  as511_status_var_insert_type( td, STATUS_VAR_MERKER, sp->sp.AddrMerker, NULL, NULL  );  // MB0

  if( as511_status_var_start( td )) {
    printf("MB0                      T1                          T0\n");
    while( 1 ) {
      as511_status_var_run( td );
      for( dl = td->dlh->l; dl; dl = dl->v ) {
        svd = DL_GET_DATA(svd_u, dl );
        switch( svd->type ) {
          case STATUS_VAR_DATEN: 
            printf("T=%-4d ZeitBasis=%d %s | ",
                   STATUS_VAR_TIMER_WERT(svd),
                   STATUS_VAR_TIMER_BASIS(svd),
                   runtimer[STATUS_VAR_TIMER_RUN(svd)]
                  );
            break;
          case STATUS_VAR_MERKER: 
            printf("BYTE=0x%02X Bit=", svd->t4.w );
            for( i = 0, bit = 0x80; i < 8; i++ ) {
              printf("%d",(svd->t4.w & bit) ? 1:0 );
              bit >>= 1;
            }
            printf(" | ");
            break;
        }
      }
      printf("\n");
      if( KbHit() )
        break;
    }
    as511_status_var_stop( td );
  }
  as511_status_var_destroy( td, NULL );

  as511_read_system_parameter_free( td, sp );

  return 1;
}

int test_status_module( td_t *td, unsigned short offset, unsigned char bsttyp, unsigned char bstnr )
{
  char *timer[] = { "Steht ", "Laeuft" };
  syspar_t  *sp;
  bal_t     *ba;
  dl_t *dl;
  smd_u *smd;
  ag_t  *ag;
  word_t load = STATUS_MODULE_LOAD;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {
    PrintSystemParameter( sp );
  }

  printf("Status Variable Starten ? Y/n\n");
  printf("Beliebige Taste Stoppt die Funktion Status Baustein\n");

  if( GetKey() == 'n' ) {
    return 0;
  }

  if( (ba = as511_read_module_addr_list( td, bsttyp )) != NULL ) {
    if( (ba->ptr[bstnr] == 0) ) {
      printf("Baustein %s%d nicht im AG\n",BstTypToString((u8)bsttyp),bstnr);
      return 0;
    }
    else {
      printf("Baustein %s%d an Adresse 0x%04X im AG\n",BstTypToString((u8)bsttyp),bstnr, ba->ptr[bstnr]);
    }
    as511_read_module_addr_list_free( td, ba );
  }

  if( (ag = as511_get_ag_typ( td, sp )) != NULL ) {
    switch( ag->ag_typ ) {
      case AG100U:
        load = STATUS_MODULE_LOAD;
        break;
      case AG135U:
      case AG155U:
        load = STATUS_MODULE_LOAD_LARGE;
        break;
      default:
        load = STATUS_MODULE_LOAD;
        break;
    }
  }

  if( as511_status_module_create( td ) == 0 ) {
    as511_status_module_insert_op( td, STATUS_MODULE_MERKER, sp->sp.AddrMerker, "UN M 0.0"  );
    as511_status_module_insert_op( td, load, 0,                                 "L  KT100.0");
    as511_status_module_insert_op( td, STATUS_MODULE_DATEN,  sp->sp.AddrZeiten, "SE T 0"    );
    as511_status_module_insert_op( td, STATUS_MODULE_DATEN,  sp->sp.AddrZeiten, "U  T 0"    );
    as511_status_module_insert_op( td, STATUS_MODULE_MERKER, sp->sp.AddrMerker, "=  M 0.0"  );
    as511_status_module_insert_op( td, STATUS_MODULE_MERKER, sp->sp.AddrMerker, "U  M 0.0"  );
    as511_status_module_insert_op( td, STATUS_MODULE_MERKER, sp->sp.AddrMerker, "UN M 0.1"  );
    as511_status_module_insert_op( td, STATUS_MODULE_NOPAR, 0,                  "O"         );
    as511_status_module_insert_op( td, STATUS_MODULE_MERKER, sp->sp.AddrMerker, "UN M 0.0"  );
    as511_status_module_insert_op( td, STATUS_MODULE_MERKER, sp->sp.AddrMerker, "U  M 0.1"  );
    as511_status_module_insert_op( td, STATUS_MODULE_MERKER, sp->sp.AddrMerker, "=  M 0.1"  );

start_sv:

    if( as511_status_module_start( td, offset, bsttyp, bstnr ) == NO_ERROR) {
      while( 1 ) {
        as511_status_module_run( td );
        if( td->errnr == 0 ) {
          for( dl = td->dlh->f; dl; dl = dl->n ) {
            smd = DL_GET_DATA(smd_u, dl );
            printf("%04X  %s \t",STATUS_MODULE_AG_ADDR(smd), (char*)dl->udata);
            switch( smd->type ) {
              case STATUS_MODULE_LOAD:
                printf("%02X %02X %04X %04X\n",
                       STATUS_MODULE_VKE(smd),
                       STATUS_MODULE_S2(smd),
                       STATUS_MODULE_AKKU1(smd),
                       STATUS_MODULE_AKKU2(smd));
                break;
              case STATUS_MODULE_LOAD_LARGE:  
                              printf("%02X %02X %08X %08X\n",
                       STATUS_MODULE_VKE(smd),
                       STATUS_MODULE_S2(smd),
                       STATUS_MODULE_AKKU1L(smd),
                       STATUS_MODULE_AKKU2L(smd));
                break;
              case STATUS_MODULE_DATEN: 
                printf("\t%02X %02X %-4d %s\n",
                       STATUS_MODULE_VKE(smd),
                       STATUS_MODULE_S2(smd),
                       STATUS_MODULE_TIMER_WERT(smd),
                       timer[STATUS_MODULE_TIMER_RUN(smd)]);
                break;
              case STATUS_MODULE_MERKER:
                printf("\t%02X %02X\n",
                       STATUS_MODULE_VKE(smd),
                       STATUS_MODULE_BYTE_VALUE(smd));
                break;
              case STATUS_MODULE_NOPAR:
                printf("\t%02X\n",
                       STATUS_MODULE_VKE(smd));
                break;
            }
          }
          printf("\n");
          if( KbHit() )
            goto ende;
        }
        else {
          printf("td->errnr = %04X\n", td->errnr);
          printf("Ist AG in RUN ? y/N\n");
          as511_status_module_stop( td );
          if( GetKey() == 'y' ) {
            goto start_sv;
          }
          as511_status_module_destroy( td, NULL );
          as511_read_system_parameter_free( td,  sp );
          return 0;
        }
      }
ende:
      as511_status_module_stop( td );
    }
    as511_status_module_destroy( td, NULL );
  }
  as511_read_system_parameter_free( td,  sp );
  return 1;
}

int test_bearbeitungskontrolle( td_t *td )
{
  char *timer[] = { "Steht ", "Laeuft" };
  dl_t  *dl;
  syspar_t  *sp;
  word_t load = STATUS_MODULE_LOAD;
  ag_t  *ag;
  smd_u *smd;

  if( (sp = as511_read_system_parameter(td)) == NULL ) {
    printf("Fehler: Systemparameter konnten nicht gelesen werden\n");
    return 0;
  }
  else {
    PrintSystemParameter( sp );
  }

  if( (ag = as511_get_ag_typ( td, sp )) != NULL ) {
    switch( ag->ag_typ ) {
      case AG100U:
        load = STATUS_MODULE_LOAD;
        break;
      case AG135U:
      case AG155U:
        load = STATUS_MODULE_LOAD_LARGE;
        break;
      default:
        load = STATUS_MODULE_LOAD;
        break;
    }
  }

  as511_step_module_create( td );

  as511_step_module_insert_op( td, DEBUG_MODULE_MERKER, sp->sp.AddrMerker, "UN M0.0");
  as511_step_module_insert_op( td, load, 0,                                "L  KT 100.1");
  as511_step_module_insert_op( td, DEBUG_MODULE_DATEN,  sp->sp.AddrZeiten, "SE T0");
  as511_step_module_insert_op( td, DEBUG_MODULE_DATEN,  sp->sp.AddrZeiten, "U  T0");
  as511_step_module_insert_op( td, DEBUG_MODULE_MERKER, sp->sp.AddrMerker, "=  M0.0");
  as511_step_module_insert_op( td, DEBUG_MODULE_MERKER, sp->sp.AddrMerker, "U  M0.0");
  as511_step_module_insert_op( td, DEBUG_MODULE_MERKER, sp->sp.AddrMerker, "UN M0.1");
  as511_step_module_insert_op( td, DEBUG_MODULE_NOPAR,  0,                 "O");
  as511_step_module_insert_op( td, DEBUG_MODULE_MERKER, sp->sp.AddrMerker, "UN M0.0");
  as511_step_module_insert_op( td, DEBUG_MODULE_MERKER, sp->sp.AddrMerker, "U  M0.1");
  as511_step_module_insert_op( td, DEBUG_MODULE_MERKER, sp->sp.AddrMerker, "=  M0.1");
  fprintf(stderr,"Bearbeitungskontrolle EIN y/N\n");
  if( GetKey() == 'y' ) {

    as511_step_module_init( td );
    fprintf(stderr,"Press n to begining and/or continue\n");
    if( GetKey() == 'n' ) {
      dl = as511_step_module_start( td, 0, OB, 1 );

      while( dl != NULL ) {
        smd = DL_GET_DATA(smd_u, dl );


        fprintf(stderr, "0x%04X ",DEBUG_MODULE_AG_ADDR(smd));
        fprintf(stderr, "%-8s \t",    (char*)dl->udata );
        switch( smd->type ) {
          case DEBUG_MODULE_LOAD:  
            printf("\t%02X %02X %04X %04X\n",
                   DEBUG_MODULE_VKE(smd),
                   DEBUG_MODULE_S2(smd),
                   DEBUG_MODULE_AKKU1(smd),
                   DEBUG_MODULE_AKKU2(smd));
            break;
          case DEBUG_MODULE_LOAD_LARGE:  
            printf("\t%02X %02X %08X %08X\n",
                   DEBUG_MODULE_VKE(smd),
                   DEBUG_MODULE_S2(smd),
                   DEBUG_MODULE_AKKU1L(smd),
                   DEBUG_MODULE_AKKU2L(smd));
            break;
          case DEBUG_MODULE_DATEN: 
                      printf("\t%02X %02X %-4d %s\n",
                   DEBUG_MODULE_VKE(smd),
                   DEBUG_MODULE_S2(smd),
                   DEBUG_MODULE_TIMER_WERT(smd),
                   timer[DEBUG_MODULE_TIMER_RUN(smd)]);
            break;
          case DEBUG_MODULE_MERKER:
            printf("\t%02X %02X\n",
                   DEBUG_MODULE_VKE(smd),
                   DEBUG_MODULE_BYTE_VALUE(smd));
            break;
          case DEBUG_MODULE_NOPAR:
            printf("\t%02X\n",
                   DEBUG_MODULE_VKE(smd));
            break;
        }

        if( GetKey() == 'n' ) {
          dl = as511_step_module_continue( td, dl->n );
        }
        else {
          break;
        }

      }
    }
    as511_step_module_stop( td );
  }

  as511_read_system_parameter_free( td, sp );
  as511_step_module_destroy( td, NULL );
  return 0;
}

int test_ausgabe_steuern( td_t *td )
{
  char *s,*n;
  int i;
  int addr;
  int value;
  copbl_t *bl = NULL;

  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  printf("\n\n\n!!!!!!!!! ALERT !!!!! ACHTUNG !!!!!!!ALERT !!!!!\n"
         "Diese Funktion Steuert Ausgänge an. Dies kann\n"
         "zu Schweren Schäden an Menschen und Maschinen führen\n\n"
         "Wollen Sie diese Funtion wirklich Ausführen ? y/N\n" );
  printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

  if( GetKey() != 'y' )
    return 0;

  if( CtrlOutp && (s = alloca(strlen(CtrlOutp)+1)) != NULL ) {
    strcpy(s,CtrlOutp);

    as511_ctrl_output_create( td );
    while( 1 ) {
      if((n = strtok( s, "," )) == NULL )
        break;

      s = NULL;
      addr = strtol(n,NULL,0);

      if((n = strtok( NULL, "," )) == NULL )
        value = 0;
      else
        value = strtol(n,NULL,0);

      printf("Adresse=AB%d Wert=%02X\n",addr, value );
      as511_ctrl_output_insert_op( td, UCHAR(addr), UCHAR(value), NULL);
    }

    if( as511_ctrl_output_init( td ) == NO_ERROR ) {
      if( (bl = as511_ctrl_output_start( td )) != NULL ) {
        for( i = 0; i < bl->badlstsize; i++ ) {
          printf("Peripherie AB %02d nicht ansprechbar\n", bl->badlst[i]);
        }
        as511_ctrl_output_bl_free( td, bl );
      }
      printf("Funktion beenden bitte Taste Betätigen\n");
      GetKey();
      as511_ctrl_output_stop( td );
    }
    else {
      switch( td->errnr ) {
        case CHAR_UNKNOWN:
          printf("Unerwartetes Zeichen von der AS\n");
          break;
        case ERROR_AG_RUNING:
          printf("Funktion nur in AG STOP erlaubt\n");
          break;
      }
    }
    as511_ctrl_output_destroy( td, NULL );
  }
  return 1;
}

int test_read_bstack( td_t *td )
{
  bstack_t *b;
  int i;
  b = as511_read_bstack(td);
  switch( td->errnr ) {
    case 0:
      if( b != NULL ) {
        printf("Ausgabe BSTACK\n");
        for( i = 0; i < b->laenge; i++ ) {
          printf("%04X\t%04X\n", b->ptr[i].dbnr, b->ptr[i].retaddr);
        }
      }
      break;
    case STACK_EMPTY:
      printf("Unvollständiger oder Leerer Stack\n");
      break;
    case ERROR_AG_RUNING:
      printf("AG nicht in Stop !\n");
      break;
    default:
      printf("ERRNR vom Protokoll = %04X\n", td->errnr);
      break;
  }
  as511_read_bstack_free( td, b );
  return 1;
}

int test_read_ustack( td_t *td )
{
  ustack_t *u;
  u = as511_read_ustack(td);
  switch( td->errnr ) {
    case 0:
      if( u != NULL ) {
        printf("Ausgabe USTACK\n");
        dumpdata( u->ptr, u->laenge );
      }
      break;
    case STACK_EMPTY:
      printf("Unvollständiger oder Leerer Stack\n");
      break;
    case ERROR_AG_RUNING:
      printf("AG nicht in Stop !\n");
      break;
    default:
      printf("ERRNR vom Protokoll = %04X\n", td->errnr);
      break;
  }
  as511_read_ustack_free( td, u );
  return 1;
}

int main( int argc, const char **argv)
{
  poptContext optCon;
  const char *poptStr1;
  const char *poptStr2;
  int rc;
  td_t *td;
  bs_t  bst;
  int bsttyp;
  ML       *ml;

  int a = 0;
  int s = 0;
  int f = 0;
  int t = 0;
  int n = 0;
  int o = 0;
  int D = 0;

  optCon = poptGetContext(NULL, argc, argv, option, 0);

  while( (rc = poptGetNextOpt(optCon)) != -1 ) {
    if(rc < -1) {
      poptStr1 = poptBadOption(optCon, POPT_BADOPTION_NOALIAS);
      poptStr2 = poptStrerror(rc);

      printf("Unerlaubte Option: %s %s\n",poptStr1,poptStr2);
      return 1;
    }

    if( rc == 0x2002 ) a = 1;
    if( rc == 0x2003 ) s = 1;
    if( rc == 0x2004 ) f = 1;
    if( rc == 0x2005 ) t = 1;
    if( rc == 0x2006 ) n = 1;
    if( rc == 0x2007 ) o = 1;
    if( rc == 0x4001 ) D = 1;
  }

  if(help) {
    poptPrintHelp(optCon, stdout, 0 );
    return 0;
  }

  if(usage || argc < 2 ) {
    poptPrintUsage(optCon, stdout, 0);
    return 0;
  }

  if( access( dev, R_OK | W_OK )  == -1 ) {
    perror(dev);
    return errno;
  }

  if( (td = open_tty(dev)) != NULL  )
  {
    if( D ) {
      td->debug_level = debug;
    }
    else {
      td->debug_level = DEBUG_LEVEL_AS511;
    }

    if( ReadSysPar ) {
      lese_system_parameter( td );
    }

    if( ReadAdrLst ) {
      if( t ) {
        read_module_addr_list(td,BstTyp);
      }
      else {
        printf("Parameter --module-type=DB ... angeben\n");
      }
    }

    if( AgRun ) {
      ag_run( td );
    }

    if( AgStop ) {
      ag_stop( td );
    }

    if( BstTrfFdAg ) {
      if( f ) {
        BstTransferFdAg( td, Mc5File );
      }
      else {
        printf("Parameter --mc5file=Dateiname angeben\n");
      }
    }

    if( BstTrfAgFd ) {
      if( t && n && f) {
        if( (bsttyp = StringToBstTyp(BstTyp)) != 0 && (BstNr >= 0 || BstNr <=255))
          BstTransferAgFd( td, UCHAR(bsttyp), UCHAR(BstNr), Mc5File );
        else {
          printf("Ungültiger Bausteintyp %s\n",BstTyp);
        }
      }
      else {
        printf("Parameter --module-type=DB ... angeben\n");
        printf("Parameter --module-nummer=XXX angeben\n");
        printf("Parameter --mc5file=Dateiname angeben\n");
      }
    }

    if( EnRam ) {
      if( a && s ) {
        test_ram( td, Start, Size );
      }
      else {
        printf("Parameter --start-mem-addr=0x???? angeben\n");
        printf("Parameter --size-mem-addr=??? angeben\n");
      }
    }

    if( EnCompress ) {
      test_compress( td );
    }

    if( ReadBstInfo ) {
      if( t && n ) {
        if( (bsttyp = StringToBstTyp(BstTyp))&& (BstNr >= 0 || BstNr <=255)) {
          read_module_info(td,UCHAR (bsttyp), UCHAR(BstNr));
        }
        else {
          printf("Ungültiger Bausteintyp %s oder\n",BstTyp);
          printf("Ungültiger Bausteinnummer %d\n",BstNr);
        }
      }
      else {
        printf("Parameter --module-type=DB ... angeben\n");
        printf("Parameter --module-nummer=XXX angeben\n");
      }
    }

    if( EnDelete ) {
      if( t && n ) {
        if( (bsttyp = StringToBstTyp(BstTyp)) == 0 ) {
          printf("Ungültiger Bausteintyp %s\n", BstTyp );
          return 3;
        }
        if( BstNr < 0 || BstNr > 255 ) {
          printf("Ungültige Bausteinnummer %d\n", BstNr );
          return 3;
        }
        test_delete(td, UCHAR (bsttyp), UCHAR (BstNr) );
      }
      else {
        printf("Parameter --module-type=DB ... angeben\n");
        printf("Parameter --module-nummer=XXX angeben\n");
      }
    }

    if( EnStatusVar ) {
      test_status_var( td, NULL );
    }

    if( EnStatusBst ) {
      as511_set_bst_data( &bst, OB, 1, sizeof(ob1), ob1 );
      if ( test_write_module( td, &bst ) )
        test_status_module( td, 0, OB, 1 );
    }

    if( StepModule ) {
      as511_set_bst_data( &bst, OB, 1, sizeof(ob1), ob1 );
        if ( test_write_module( td, &bst ) )
          test_bearbeitungskontrolle(td);
    }

    if( o ) {
      test_ausgabe_steuern( td );
    }

    if( ReadBstack ) {
      test_read_bstack( td );
    }

    if( ReadUstack ) {
      test_read_ustack( td );
    }

    close_tty(td);
  }

  printf("MallocDebugZähler = %d\n",MallocZaehler);
  if( md.debug > 0 ) {
    for( ml = md.mlf; ml; ml = ml->n ) {
      printf("size = %6d prt = %p   ", (int) ml->size, (void*)ml->ptr);
      printf("isfree = %d\n", ml->isfree);
    }
  }
  return 0;
}
