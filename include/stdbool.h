#ifndef _STDBOOL_H_
#define _STDBOOL_H_

#ifndef __cplusplus

#if defined(_MSC_VER) && (_MSC_VER < 1800)
typedef enum { _Bool_must_promote_to_int = -1, false = 0, true = 1 } _Bool;
#endif

#define bool _Bool
#define false   0
#define true    1

#endif /* __cplusplus */

#define __bool_true_false_are_defined 1

#endif /* _STDBOOL_H_ */
