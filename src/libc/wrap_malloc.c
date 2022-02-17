#include <errno.h>
#include <stddef.h>
#include <reent.h>
#include <string.h>

// extern void *LOS_MemAlloc(void *pool, size_t size);
// extern size_t LOS_MemFree(void *pool, void *ptr);
// extern void *LOS_MemRealloc(void *pool, void *ptr, size_t size);

extern void *rt_malloc(size_t size);
extern void *rt_calloc(size_t nmemb, size_t size);
extern void *rt_free(void *ptr);
extern void *rt_realloc(void *ptr, size_t size);

extern unsigned char *m_aucSysMem0;

void *__wrap__malloc_r(struct _reent *r, size_t size)
{
    void *res = rt_malloc(size);
    if (size && !res)
        r->_errno = ENOMEM;
    return res;
}

void * __wrap__realloc_r(struct _reent *r, void *ptr, size_t size)
{
    void *res = rt_realloc(ptr, size);

    if (ptr && size && !res)
        r->_errno = ENOMEM;
    return res;
}

void *__wrap__calloc_r(struct _reent *r, size_t n, size_t size)
{
    void *res = rt_calloc(n, size);

	return res;
}

void __wrap__free_r(struct _reent *r, void *ptr)
{
    rt_free(ptr);
}
