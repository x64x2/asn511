#ifndef __AS511_MEM_H__
#define __AS511_MEM_H__

#include <cstddef>
#define DL_LOCK   1
#define DL_UNLOCK 0

#define MEM_NO_DATA  1
#define MEM_BAD_TYPE 2
#define MEM_NO_ERROR 0

#define DL_TYPE_STATUS_VAR    1
#define DL_TYPE_STATUS_MODULE 2
#define DL_TYPE_CTRL_OUTPUT   4
#define DL_TYPE_STEP_MODULE   8

#define DL_GET_DATA(d_type, d ) ((d_type*) (d)->data )

struct MallocList
{
  struct MallocList *v;
  struct MallocList *n;
  int                ptr;
  size_t             size;
  int                isfree;
};
typedef struct MallocList ML;

struct MallocDebug
{
  int debug;
  ML *mlf; 
  ML *mll; 
};
typedef struct MallocDebug MD;

struct dbl_list_head
{
  struct dbl_list *f;  
  struct dbl_list *l;  

  int    dl_type;   
  int    dl_lock;    
  void   *data;        
};
typedef struct dbl_list_head dlf_t;  
typedef struct dbl_list_head dlh_t;

struct dbl_list
{
  struct dbl_list *v;
  struct dbl_list *n; 
  void  *data;         
  void  *udata;       
};
typedef struct dbl_list dl_t;

dl_t *dlh_insert_last ( dlh_t *dlh );
dl_t *dlh_insert_first( dlh_t *dlh );
int   dl_insert_data( dlh_t *dlh, dl_t *dl, int dl_type, void *data, size_t ds, void *udata );

dlh_t *dlh_create ( int dl_type );
void * dlh_delete ( dlh_t *dlh, dl_t *dl, int dl_type, int (*usrfk)(void*), void *ud);
dl_t  *dl_create  ( void );

int dl_print_data( dl_t *dl );

#endif
