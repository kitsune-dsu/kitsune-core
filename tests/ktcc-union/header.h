#ifndef header
#define header

typedef void (*PreprocPostConfigFunc)(void *);
typedef struct _PreprocPostConfigFuncNode
{
    void *data;
    union
    {
        PreprocPostConfigFunc fptr;
        void *void_fptr;
    } unionfptr;
    struct _PreprocPostConfigFuncNode *next;

} PreprocPostConfigFuncNode;

#endif

