#ifndef LOKI_MACRO_CONCATENATE_INC_
#define LOKI_MACRO_CONCATENATE_INC_

// $Id$

/** @note This header file provides a common definition of macros used to
 concatenate names or numbers together into a single name or number.
 */

#define LOKI_CONCATENATE_DIRECT(s1, s2) s1##s2
#define LOKI_CONCATENATE(s1, s2) LOKI_CONCATENATE_DIRECT(s1, s2)

#endif