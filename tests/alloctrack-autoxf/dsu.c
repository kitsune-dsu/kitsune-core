#define E_NOANNOT
#include <kitsune.h>
#include <transform.h>
#include <assert.h>
#include <stdlib.h>
#include "../../src/alloctrack_internal.h"
__attribute__((constructor)) void _kitsune_register_renames() {
}
struct _kitsune_old_rename_range;
struct _kitsune_new_rename_range;
struct _kitsune_old_rename_range {
  long start;
  long end;
};
struct _kitsune_new_rename_range {
  long start;
  long end;
  long data;
};



//TODO This will make a segfault on delayed free...where to do kitsune malloc/free instead?
//TODO write this
void _kitsune_transform_struct_range_to_struct_range(void *arg_in,void *arg_out,int num_args,void **args) {
  struct _kitsune_old_rename_range (*local_in) = arg_in;
  struct _kitsune_new_rename_range (*local_out) = arg_out;
  assert(num_args == 0);
  XF_INVOKE(XF_RAW(sizeof(long )), &((*local_in).start), &((*local_out).start));
  XF_INVOKE(XF_RAW(sizeof(long )), &((*local_in).end), &((*local_out).end));
  { local_out->data= 0; }
  
}
void _kitsune_transform_my_struct_array_ten() {
  int i;
  struct _kitsune_old_rename_range (*(*local_in));
  local_in = kitsune_get_symbol_addr_old("my_struct_array_ten", 0, 0, 0);
  assert(local_in);
  struct _kitsune_new_rename_range (*(*local_out));
  local_out = kitsune_get_symbol_addr_new("my_struct_array_ten", 0, 0, 0);
  assert(local_out);
  printf("Looking up stuff at addrs: %p\n",*local_in);
  void * node_addr = alloctrack_lookup(*local_in);
  if(node_addr){

    printf("found array_ten at %p\n", node_addr);

    // update the new address range in the interval tree  
    //TODO this will be kitsune_malloc followed by kitsune_delayed_free or something...
    //kitsune_migrate_alloced_track(local_out, local_out+(10*(sizeof(struct _kitsune_new_rename_range))) , *local_in); 

    //manual for now until xfgen working.
    *local_out = kitsune_malloc(sizeof(struct _kitsune_new_rename_range)*10); 
    for(i=0;i<10;i++)
      memcpy((*local_out)+i,(*local_in)+i, sizeof(struct _kitsune_old_rename_range));

    //TODO add to delayed free once I can specify kitsune_free versus regular free...  
    kitsune_delayedfree(alareas_get_start(node_addr));
    
  }
  else{  // proceed normally
    printf("found nothing at %p\n",local_in );
    XF_INVOKE(XF_PTRARRAY(10, sizeof(struct _kitsune_old_rename_range ), sizeof(struct _kitsune_new_rename_range ), XF_LIFT(_kitsune_transform_struct_range_to_struct_range, XF_DEEP, sizeof(struct _kitsune_old_rename_range ), sizeof(struct _kitsune_new_rename_range ))), local_in, local_out);
  }
}
void _kitsune_transform_my_struct_array_one() {
  struct _kitsune_old_rename_range (*(*local_in));
  local_in = kitsune_get_symbol_addr_old("my_struct_array_one", 0, 0, 0);
  assert(local_in);
  struct _kitsune_new_rename_range (*(*local_out));
  local_out = kitsune_get_symbol_addr_new("my_struct_array_one", 0, 0, 0);
  assert(local_out);
  printf("Looking up stuff at addrs: %p\n",*local_in);

  /* Step 1) if the address is in the interval tree, get the start of the interval (->start)*/
  struct mem_area * addr = alloctrack_lookup(*local_in);


  if(addr){

    /* Step 2) then get the name of the symbol at the start of the interval*/
    //TODO why is this not working?!?!
    //  char *symbol = kitsune_lookup_addr_old((addr->start)); 
    ////  printf("found array_one head at %p (%p), part of  %s, but should have looked at %p\n", (addr->start), correct, kitsune_lookup_addr_old(correct), (*(struct _kitsune_old_rename_range *)correct));
    char * symbol = "my_struct_array_ten";

    /* Step 3) then get the new address of where the symbol of the start of the old interval is in the new space */
    /* TODO important: order is going to matter here.  array_ten must be xformed before array_one*/
//    void * new_array_ten = kitsune_get_symbol_addr_new(symbol, 0, 0, 0);
    struct _kitsune_new_rename_range ** new_array_ten = kitsune_get_symbol_addr_new(symbol, 0, 0, 0);


    /* Step 4) reposition this structure inside the new address for the containing array */
//    XF_INVOKE(XF_PTR(XF_LIFT(_kitsune_transform_struct_range_to_struct_range, XF_DEEP, sizeof(struct _kitsune_old_rename_range ), sizeof(struct _kitsune_new_rename_range ))), local_in, local_out);
     *local_out = ((*new_array_ten)+3);
  }
  /* the address is not in the interval tree, proceed normally*/
  else{  
    printf("found nothing at %p\n",local_in );
    XF_INVOKE(XF_PTR(XF_LIFT(_kitsune_transform_struct_range_to_struct_range, XF_DEEP, sizeof(struct _kitsune_old_rename_range ), sizeof(struct _kitsune_new_rename_range ))), local_in, local_out);
  }
}



