
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>


/*
> Suppose an update to this program changes the representation of T, so that it needs to be copied. Then xfgen would follow the g pointer, malloc() a new object and copy into it, then traverse the x pointer and do the same, breaking the aliasing relationship.
*/
struct range {
  long start;
  long end;
  long data;
};

struct range * E_PTRARRAY(10) my_struct_array_ten;
struct range *my_struct_array_one;

int main(int argc, char **argv) 
{

  kitsune_do_automigrate(); 
//  MIGRATE_GLOBAL(my_struct_array_ten);
//  MIGRATE_GLOBAL(my_struct_array_one);
  
  printf("|----- UPDATE & AUTOMIGRATE -----|\n");
  printf("B:  sizeof(struct range)=%d  \n",sizeof(struct range));
  my_struct_array_one->end = 24;

  printf("other.c:    my_struct_array_ten at %p\n", my_struct_array_ten);
  printf("other.c: my_struct_array_ten[3] at %p\n", &my_struct_array_ten[3]);
  printf("other.c:    my_struct_array_one at %p\n", my_struct_array_one);
  printf("other.c: my_struct_array_ten[3]->end: %d\n", (struct range*)(&my_struct_array_ten[3])->end );
  printf("other.c:    my_struct_array_one->end: %d\n", my_struct_array_one->end);

  assert(my_struct_array_one->end == 24);
  assert(((struct range*)(&my_struct_array_ten[3])->end) == 24);
  printf("C:  sizeof(struct range)=%d  \n",sizeof(struct range));
  printf("Sucesss...\n");

  kitsune_update("test");

  return 0;
}
