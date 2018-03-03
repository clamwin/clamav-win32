#ifndef _STDBOOL_H_
#define _STDBOOL_H_

#ifndef __GNUC__
typedef enum { _Bool_must_promote_to_int = -1, false = 0, true = 1 } _Bool;
#endif

#define bool _Bool

#endif /* _STDBOOL_H_ */
