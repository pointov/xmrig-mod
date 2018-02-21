#ifndef _SKEIN_H_
#define _SKEIN_H_     1

#include "skein_port.h"

typedef enum
{
  SKEIN_SUCCESS         =      0,
  SKEIN_FAIL            =      1,
  SKEIN_BAD_HASHLEN     =      2
}
SkeinHashReturn;

typedef size_t   SkeinDataLength;
typedef u08b_t   SkeinBitSequence;

SkeinHashReturn skein_hash(int hashbitlen,   const SkeinBitSequence *data,
        SkeinDataLength databitlen, SkeinBitSequence *hashval);

void xmr_skein(const SkeinBitSequence *data, SkeinBitSequence *hashval);

#endif  /* ifndef _SKEIN_H_ */
