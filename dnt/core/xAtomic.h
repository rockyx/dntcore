#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XATOMIC_H__
#define __DNT_CORE_XATOMIC_H__

#include <dnt/core/xGlobal.h>

X_CORE_BEGIN_DECLS

xInt x_atomic_int_get(const volatile xInt *atomic);
void x_atomic_int_set(volatile xInt *atomic, xInt newval);
void x_atomic_int_inc(volatile xInt *atomic);
xBoolean x_atomic_int_dec_and_test(volatile xInt *atomic);
xBoolean x_atomic_int_compare_and_exchange(volatile xInt *atomic, xInt oldval, xInt newval);
xInt x_atomic_int_add(volatile xInt *atomic, xInt val);
xUInt x_atomic_int_and(volatile xUInt *atomic, xUInt val);
xUInt x_atomic_int_or(volatile xUInt *atomic, xUInt val);
xUInt x_atomic_int_xor(volatile xUInt *atomic, xUInt val);
xPointer x_atomic_pointer_get(const volatile void* atomic);
void x_atomic_pointer_set(volatile void *atomic, xPointer newval);
xBoolean x_atomic_pointer_compare_and_exchange(volatile void* atoimc, xPointer oldval, xPointer newval);
xSSize x_atomic_pointer_add(volatile void *atomic, xSSize val);
xSize x_atomic_pointer_and(volatile void *atomic, xSize val);
xSize x_atomic_pointer_or(volatile void *atomic, xSize val);
xSize x_atomic_pointer_xor(volatile void *atomic, xSize val);

X_CORE_END_DECLS

#if defined(X_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

#define x_atomic_int_get(atomic) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ *(atomic) : 0);                                  \
	__sync_synchronize ();                                                   \
	(xInt) *(atomic);                                                        \
}))
#define x_atomic_int_set(atomic, newval) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ (newval) : 0);                                   \
	*(atomic) = (newval);                                                    \
	__sync_synchronize ();                                                   \
}))
#define x_atomic_int_inc(atomic) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ *(atomic) : 0);                                  \
	(void) __sync_fetch_and_add ((atomic), 1);                               \
}))
#define x_atomic_int_dec_and_test(atomic) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ *(atomic) : 0);                                  \
	__sync_fetch_and_sub ((atomic), 1) == 1;                                 \
}))
#define x_atomic_int_compare_and_exchange(atomic, oldval, newval) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ (newval) ^ (oldval) : 0);                        \
	(xBoolean) __sync_bool_compare_and_swap ((atomic), (oldval), (newval));  \
}))
#define x_atomic_int_add(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ (val) : 0);                                      \
	(xInt) __sync_fetch_and_add ((atomic), (val));                           \
}))
#define x_atomic_int_and(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ (val) : 0);                                      \
	(xUInt) __sync_fetch_and_and ((atomic), (val));                          \
}))
#define x_atomic_int_or(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ (val) : 0);                                      \
	(xUInt) __sync_fetch_and_or ((atomic), (val));                           \
}))
#define x_atomic_int_xor(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xInt));                     \
	(void) (0 ? *(atomic) ^ (val) : 0);                                      \
	(xUInt) __sync_fetch_and_xor ((atomic), (val));                          \
}))

#define x_atomic_pointer_get(atomic) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xPointer));                 \
	__sync_synchronize ();                                                   \
	(xPointer) *(atomic);                                                    \
}))
#define x_atomic_pointer_set(atomic, newval) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xPointer));                 \
	(void) (0 ? (xPointer) *(atomic) : 0);                                   \
	*(atomic) = (__typeof__ (*(atomic))) (xSize) (newval);                   \
	__sync_synchronize ();                                                   \
}))
#define x_atomic_pointer_compare_and_exchange(atomic, oldval, newval) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xPointer));                 \
	(void) (0 ? (xPointer) *(atomic) : 0);                                   \
	(xBoolean) __sync_bool_compare_and_swap ((atomic), (oldval), (newval));  \
}))
#define x_atomic_pointer_add(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xPointer));                 \
	(void) (0 ? (xPointer) *(atomic) : 0);                                   \
	(void) (0 ? (val) ^ (val) : 0);                                          \
	(xSSize) __sync_fetch_and_add ((atomic), (val));                         \
}))
#define x_atomic_pointer_and(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xPointer));                 \
	(void) (0 ? (xPointer) *(atomic) : 0);                                   \
	(void) (0 ? (val) ^ (val) : 0);                                          \
	(xSize) __sync_fetch_and_and ((atomic), (val));                          \
}))
#define x_atomic_pointer_or(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xPointer));                 \
	(void) (0 ? (xPointer) *(atomic) : 0);                                   \
	(void) (0 ? (val) ^ (val) : 0);                                          \
	(xSize) __sync_fetch_and_or ((atomic), (val));                           \
}))
#define x_atomic_pointer_xor(atomic, val) \
	(X_GNUC_EXTENSION ({                                                          \
	X_STATIC_ASSERT (sizeof *(atomic) == sizeof (xPointer));                 \
	(void) (0 ? (xPointer) *(atomic) : 0);                                   \
	(void) (0 ? (val) ^ (val) : 0);                                          \
	(xSize) __sync_fetch_and_xor ((atomic), (val));                          \
}))

#else // defined(X_ATOMIC_LOCK_FREE) && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

#define x_atomic_int_get(atomic) \
	(x_atomic_int_get ((xInt *) (atomic)))
#define x_atomic_int_set(atomic, newval) \
	(x_atomic_int_set ((xInt *) (atomic), (xInt) (newval)))
#define x_atomic_int_compare_and_exchange(atomic, oldval, newval) \
	(x_atomic_int_compare_and_exchange ((xInt *) (atomic), (oldval), (newval)))
#define x_atomic_int_add(atomic, val) \
	(x_atomic_int_add ((xInt *) (atomic), (val)))
#define x_atomic_int_and(atomic, val) \
	(x_atomic_int_and ((xUInt *) (atomic), (val)))
#define x_atomic_int_or(atomic, val) \
	(x_atomic_int_or ((xUInt *) (atomic), (val)))
#define x_atomic_int_xor(atomic, val) \
	(x_atomic_int_xor ((xUInt *) (atomic), (val)))
#define x_atomic_int_inc(atomic) \
	(x_atomic_int_inc ((xInt *) (atomic)))
#define x_atomic_int_dec_and_test(atomic) \
	(x_atomic_int_dec_and_test ((xInt *) (atomic)))

#define x_atomic_pointer_get(atomic) \
	(x_atomic_pointer_get (atomic))
#define x_atomic_pointer_set(atomic, newval) \
	(x_atomic_pointer_set ((atomic), (xPointer) (newval)))
#define x_atomic_pointer_compare_and_exchange(atomic, oldval, newval) \
	(x_atomic_pointer_compare_and_exchange ((atomic), (xPointer) (oldval), (xPointer) (newval)))
#define x_atomic_pointer_add(atomic, val) \
	(x_atomic_pointer_add ((atomic), (xSSize) (val)))
#define x_atomic_pointer_and(atomic, val) \
	(x_atomic_pointer_and ((atomic), (xSize) (val)))
#define x_atomic_pointer_or(atomic, val) \
	(x_atomic_pointer_or ((atomic), (xSize) (val)))
#define x_atomic_pointer_xor(atomic, val) \
	(x_atomic_pointer_xor ((atomic), (xSize) (val)))

#endif // defined(__GNUC__) && defined(X_ATOMIC_OP_USE_GCC_BUILTINS)

#endif // __DNT_CORE_XATOMIC_H__
