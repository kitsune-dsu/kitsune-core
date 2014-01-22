#ifndef VMAREAS_INTERNAL_H
#define VMAREAS_INTERNAL_H

#include <stdbool.h>

struct vmarea;
typedef struct vmarea vmarea;
typedef enum { STACK, HEAP, LIBRARY, OTHER, UNMAPPED } vmarea_type;

void vmareas_free(void);
void vmareas_init(void);

vmarea *vmareas_lookup(void *addr);

int vmareas_get_readable(vmarea *);
int vmareas_get_writable(vmarea *);
int vmareas_get_executable(vmarea *);
char *vmareas_get_label(vmarea *);
vmarea_type vmareas_get_type(vmarea *);
char *vmareas_to_str(vmarea *a);

#endif
