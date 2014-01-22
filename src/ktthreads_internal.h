#ifndef EK_THREADS_INTERNAL
#define EK_THREADS_INTERNAL

void ktthread_init(void);
int  ktthread_is_main(void);

void ktthread_main_wait(void);
void ktthread_do_update(const char *pt_name);
void ktthread_launch_wait(void);
void ktthread_finish_update(void);
void ktthread_main_finish_update(void);

int  ktthread_has_reached_update(const char *pt_name);

void **ktthread_get_top(void);
void **ktthread_get_top_old(void);

void ktthread_lock(void);
void ktthread_unlock(void);
void ktthread_rapidq(void);

#endif /* EK_THREADS_INTERNAL */
