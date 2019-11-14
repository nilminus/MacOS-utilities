#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc/malloc.h>

typedef struct interpose_s {
  void *new_func;
  void *old_func;
} interpose_t;

void *my_malloc(size_t size);
void my_free(void *);

__attribute__((used)) static const interpose_t interposing_functions[] \
    __attribute__((section("__DATA, __interpose"))) = {
      { (void *)my_free, (void *)free },
      { (void *)my_malloc, (void *)malloc },
    };

void *my_malloc (size_t size)
{
  void *returned = malloc(size);
  malloc_printf( "ALLOC: %p %d\n", returned, size);
  return (returned);
}

void my_free (void *freed)
{
  free(freed);
  malloc_printf ( "FREED: %p\n", freed);
}
