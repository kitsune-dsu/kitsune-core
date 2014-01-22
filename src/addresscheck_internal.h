#ifndef ADDRESSCHECK_INTERNAL_H_
#define ADDRESSCHECK_INTERNAL_H_

void addresscheck(char *descriptor, void *addr, size_t size);

void addresscheck_init(void);
void addresscheck_free(void);

#endif
