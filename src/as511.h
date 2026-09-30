#ifndef _AS511_H_
#define _AS511_H_

#define DB 0x01
#define SB 0x02
#define PB 0x04
#define FX 0x05
#define FB 0x08
#define DX 0x0C
#define OB 0x10
#define TB 0x20

#define SD_STOZUS 0x80
#define SD_STOANZ 0x40
#define SD_NEUSTA 0x20
#define SD_AF     0x04

#define S5_WRITE_MEM             0x03
#define S5_READ_MEM              0x04
#define S5_WRITE_BST             0x05
#define S5_READ_BST              0x06
#define S5_KOMPR_RAM             0x07
#define S5_WRITE_DB              0x08
#define S5_DELETE_MODULE         0x09
#define UNBEK0A               0x0A
#define UNBEK0B               0x0B
#define UNBEK0C               0x0C
#define UNBEK0D               0x0D
#define S5_DEBUG_START           0x0E
#define UNBEK0F               0x0F
#define S5_DEBUG_CONTINUE        0x10
#define S5_DELETE_MODULE_ALL     0x11
#define UNBEK12               0x12
#define S5_CTRL_OUTPUT           0x13
#define S5_STATUS_VAR            0x14
#define S5_STATUS_BST            0x15
#define S5_DEBUG_INIT            0x16
#define S5_CTRL_OUTPUT_INIT      0x17
#define S5_READ_SYSPAR           0x18
#define S5_READ_RAM_INFO         0x19
#define S5_READ_BOOKMARKER       0x1A
#define S5_READ_BST_ADDR_LIST    0x1B
#define S5_READ_BSTACK           0x1C
#define S5_READ_USTACK           0x1D
#define S5_CH_OP_MODE            0x1E

#define S5_ONLINE_START          0x80
#define S5_ONLINE_STOP           0x81
#define S5_CH_OP_MODE_STOP       0x00
#define S5_CH_OP_MODE_RESTART    0x01
#define S5_CH_OP_MODE_REBOOT     0x02
#define STATUS_VAR_PAE           0x30
#define STATUS_VAR_PAA           0x31
#define STATUS_VAR_MERKER        0x32
#define STATUS_VAR_ZAEHLER       0x33
#define STATUS_VAR_DATEN         0x34
#define STATUS_MODULE_PAE        0x30
#define STATUS_MODULE_PAA        0x31
#define STATUS_MODULE_MERKER     0x32

#define STATUS_MODULE_ZAEHLER    0x33
#define STATUS_MODULE_DATEN      0x34
#define STATUS_MODULE_NOPAR      0x35
#define STATUS_MODULE_LOAD       0x0036
#define STATUS_MODULE_LOAD_LARGE 0x0136
#define DEBUG_MODULE_PAE        0x30
#define DEBUG_MODULE_PAA        0x31
#define DEBUG_MODULE_MERKER     0x32
#define DEBUG_MODULE_ZAEHLER    0x33
#define DEBUG_MODULE_DATEN      0x34
#define DEBUG_MODULE_NOPAR      0x35
#define DEBUG_MODULE_LOAD       0x0036
#define DEBUG_MODULE_LOAD_LARGE 0x0136

struct baustein_kopf
{
  unsigned char  baustein_sync1;    
  unsigned char  baustein_sync2;     
  struct
  {
    unsigned int  btyp :6;          
    unsigned int  bok  :2;           
  } __attribute__((packed)) baustein_typ;
  unsigned char  baustein_nummer;     
  unsigned char  pg_kennung;          
  unsigned char  bib_nummer1;         
  unsigned char  bib_nummer2;       
  unsigned char  bib_nummer3;       
  unsigned short laenge;             
}__attribute__((packed));

typedef struct baustein_kopf bs_kopf_t;

struct buchhalter
{
  unsigned short ram_adresse;        
  unsigned char  baustein_sync1;   
  unsigned char  baustein_sync2;    
  struct
  {
    unsigned int  btyp :6;          
    unsigned int  bok  :2;          
  }__attribute__((packed)) bst;
  unsigned char  baustein_nummer;   
  unsigned char  pg_kennung;          
  unsigned char  bib_nummer1;       
  unsigned char  bib_nummer2;         
  unsigned char  bib_nummer3;        
  unsigned short laenge;              
}__attribute__((packed));            
typedef struct buchhalter buchhalter_t;
typedef struct buchhalter modinfo_t;

struct sps_system_parameter
{
  unsigned short AddrESF;          
  unsigned short AddrASF;          
  unsigned short AddrPAE_Digital;   
  unsigned short AddrPAA_Digital; 
  unsigned short AddrMerker;        
  unsigned short AddrZeiten;        
  unsigned short AddrZaehler;      
  unsigned short AddrSystemDaten;  
  unsigned char  AG_sw_version;     
  unsigned char  StatusKennung;    
  unsigned short AddrEndRam;       
  unsigned short SystemProgRam;     
  unsigned short Laenge_DB_liste;   
  unsigned short Laenge_SB_Liste;   
  unsigned short Laenge_PB_Liste; 
  unsigned short Laenge_FB_Liste;
  unsigned short Laenge_OB_Liste; 
  unsigned short Laenge_FX_Liste;   
  unsigned short Laenge_DX_Liste;  
  unsigned short Laenge_DB0_Liste;  
  unsigned char  CPU_Kennung2;     
  unsigned char  Steckplatzkenng;                                       
  unsigned short BstKopfLaenge;    
  unsigned char  unbek_7;        
  unsigned char  CPU_Kennung;      
  unsigned short unbek_8;           
  unsigned short unbek_9;      
  unsigned short unbek_10;          
} __attribute__((packed));
typedef struct sps_system_parameter sp_t;


struct syspar
{
  unsigned long laenge;
  sp_t          sp;
}__attribute__((packed));
typedef struct syspar syspar_t;

union sd  
{
  unsigned short word; 

  struct                
  {
    unsigned char byte0;
    unsigned char byte1;
  } byte;

  struct                  
  {
    unsigned int u0:1;
    unsigned int u1:1;
    unsigned int u2:1;
    unsigned int u3:1;
    unsigned int u4:1;
    unsigned int u5:1;
    unsigned int u6:1;
    unsigned int u7:1;

    unsigned int u8:1;
    unsigned int u9:1;
    unsigned int u10:1;
    unsigned int u11:1;
    unsigned int u12:1;
    unsigned int u13:1;
    unsigned int u14:1;
    unsigned int u15:1;
  } bit __attribute__((packed));
} __attribute__((packed));

struct timer
{
  unsigned int wert:10;
  unsigned int fr:1;    
  unsigned int fl:1;    
  unsigned int basis:2; 
  unsigned int unb0:1;
  unsigned int run:1;   
}__attribute__((packed));

struct zaehler
{
  unsigned int wert:1;
  unsigned int fr:1;  
  unsigned int set:1;  
  unsigned int zr:1;  
  unsigned int zv:1;    
  unsigned int unb1:1; 
  unsigned int run:1;  
} __attribute__((packed));

#endif
