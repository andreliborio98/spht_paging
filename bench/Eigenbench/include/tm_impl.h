#ifndef TM_IMPL_H_GUARD
#define TM_IMPL_H_GUARD

// TODO: these macros must be implemented by the TM lib
// a suggestion is to #undef #define in other header

#define EB_MALLOC(size) malloc(size)
#define EB_FREE(ptr)    free(ptr)

/* TODO: HTM clears the seed on abort */
#define EB_TM_BEGIN() /* TODO */
#define EB_TM_END()   /* TODO */
#define EB_TM_READ(addr)       *(addr) /* TODO */
#define EB_TM_WRITE(addr, val) *(addr) = (val) /* TODO */

#define EB_TM_INIT(nb_thrs)  /* called in the beginning of the benchmark */
#define EB_TM_DESTROY()      /* after threads exit */
#define EB_THREAD_ENTER(tid) /* on thread enter */
#define EB_THREAD_EXIT(tid)  /* on thread exit */

#endif /* TM_IMPL_H_GUARD */
