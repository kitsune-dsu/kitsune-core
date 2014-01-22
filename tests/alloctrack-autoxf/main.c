
#include <unistd.h>
#include <kitsune.h>
#include <assert.h>


struct range {
  long start;
  long end;
};

struct range * E_PTRARRAY(10) my_struct_array_ten;
struct range *my_struct_array_one;


void init(){

  my_struct_array_ten = kitsune_malloc(sizeof(struct range)*10);
  my_struct_array_one = &my_struct_array_ten[3];
  my_struct_array_ten[3].start = 17;
  my_struct_array_ten[3].end = 21;
  printf("main.c:    my_struct_array_ten at %p\n", my_struct_array_ten);
  printf("main.c: my_struct_array_ten[3] at %p\n", &my_struct_array_ten[3]);
  printf("main.c:    my_struct_array_one at %p\n", my_struct_array_one);
  printf("main.c: my_struct_array_ten[3]->end: %d\n", (struct range*)(&my_struct_array_ten[3])->end );
  printf("main.c:    my_struct_array_one->end: %d\n", my_struct_array_one->end);

}

int main(int argc, char **argv) 
{
  init();
  kitsune_do_automigrate();

  while(1){
    kitsune_update("test");
  
    if (!kitsune_is_updating()) {
      printf("A:  sizeof(struct range)=%d  \n",sizeof(struct range));
      kitsune_signal_update();    
      kitsune_set_next_version(strdup(argv[1]));
    }
  }

  return 0;
}
