#ifndef __ALIGNED_MALLOC_H__
#define __ALIGNED_MALLOC_H__


#include <stdlib.h>


#ifndef __cplusplus
extern int posix_memalign(void **__memptr, size_t __alignment, size_t __size);
#else
extern "C" int posix_memalign(void **__memptr, size_t __alignment, size_t __size);
#endif


static __inline__ void *__attribute__((__always_inline__, __malloc__)) _mm_malloc(size_t __size, size_t __align)
{
  if (__align == 1) {
    return malloc(__size);
  }

  if (!(__align & (__align - 1)) && __align < sizeof(void *))
    __align = sizeof(void *);

  void *__mallocedMemory;
  if (posix_memalign(&__mallocedMemory, __align, __size)) {
    return 0;
  }

  return __mallocedMemory;
}


static __inline__ void __attribute__((__always_inline__)) _mm_free(void *__p)
{
  free(__p);
}

#endif /* __ALIGNED_MALLOC_H__ */
