#ifndef EKIDEN_STATICVARS_H_
#define EKIDEN_STATICVARS_H_

#include <stddef.h>

void kitsune_register_var(const char *var_name, const char *funcname, 
                         const char *filename, const char *namespace,
                         void* var_addr, size_t size, int auto_migrate);
void kitsune_do_automigrate(void);

#ifdef E_NOANNOT
#define E_AUTO_MIGRATE
#define E_MANUAL_MIGRATE
#else
/**
 * \addtogroup public
 * @{
 */ 
/**
 * Annotation to instruct the Kitsune compiler to mark a variable for
 * automigration. This is most useful if you're running with automigration
 * disabled.
 */ 
#define E_AUTO_MIGRATE __attribute__((e_auto_migrate))
/**
 * Annoation to instruct the Kitsune compiler to NOT automigrate a variable in
 * kitsune_do_automigrate. The variable will not be migrated unless it is passed
 * to MIGRATE_GLOBAL. See \ref manual.
 */ 
#define E_MANUAL_MIGRATE __attribute__((e_manual_migrate))
#endif
/** @} */
void *kitsune_lookup_key_new(const char *key);
void *kitsune_lookup_key_old(const char *key);

char *kitsune_lookup_addr_new(void *addr);
char *kitsune_lookup_addr_old(void *addr);

#endif
