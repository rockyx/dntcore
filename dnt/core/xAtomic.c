#include "xAtomic.h"

#ifdef X_ATOMIC_LOCK_FREE

#if defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)

xInt (x_atomic_int_get) (const volatile xInt *atomic)
{
	return x_atomic_int_get(atomic);
}

void (x_atomic_int_set)(volatile xInt *atomic, xInt newval)
{
	x_atomic_int_set(atomic, newval);
}

void (x_atomic_int_inc)(volatile xInt *atomic)
{
	x_atomic_int_inc(atomic);
}

xBoolean (x_atomic_int_dec_and_test)(volatile xInt *atomic)
{
	return x_atomic_int_dec_and_test(atomic);
}

xBoolean (x_atomic_int_compare_and_exchange)(volatile xInt *atomic, 
	xInt oldval, 
	xInt newval)
{
	return x_atomic_int_compare_and_exchange(atomic, oldval, newval);
}

xInt (x_atomic_int_add)(volatile xInt *atomic, xInt val)
{
	return x_atomic_int_add(atomic, val);
}

XUInt (x_atomic_int_and)(volatile XUint *atomic, xUInt val)
{
	return x_atomic_int_and(atomic, val);
}

XUInt (x_atomic_int_or)(volatile XUInt *atomic, XUInt val)
{
	return x_atomic_int_or(atomic, val);
}

XUInt (x_atomic_int_xor)(volatile xUInt *atomic, XUInt val)
{
	return x_atomic_int_xor(atomic, val);
}

xPointer (x_atomic_pointer_get)(const volatile void *atomic)
{
	return x_atomic_pointer_get((const volatile xPointer*)atomic);
}

void (x_atomic_pointer_set)(volatile void *atomic, xPointer newval)
{
	x_atomic_pointer_set((volatile xPointer*)atomic, newval);
}

xBoolean (x_atomic_pointer_compare_and_exchange)(volatile void *atomic, 
	xPointer oldval, 
	xPointer newval)
{
	return x_atomic_pointer_compare_and_exchange((volatile xPointer*)atomic, oldval, newval);
}

xSSize (x_atomic_pointer_add)(volatile void *atomic, xSSize val)
{
	return x_atomic_pointer_add((volatile xPointer*)atomic, val);
}

xSize (x_atomic_pointer_and)(volatile void *atomic, xSize val)
{
	return x_atomic_pointer_and((volatile xPointer*)atomic, val);
}

xSize (x_atomic_pointer_or)(volatile void *atomic, xSize val)
{
	return x_atomic_pointer_or((volatile xPointer*)atomic, val);
}

xSize (x_atomic_pointer_xor)(volatile void *atomic, xSize val)
{
	return x_atomic_pointer_xor((volatile xPointer*)atomic, val);
}

#elif defined (X_OS_WIN)

#include <Windows.h>
#include <intrin.h>

#if !defined(X_OS_WIN64) && !(defined _MSC_VER && _MSC_VER <= 1200)
//#define InterlockedAnd _InterlockedAnd
//#define InterlockedOr _InterlockedOr;
//#define InterlockedXor _InterlockedXor
#endif

#if !defined(_MSC_VER) || _MSC_VER <= 1200

// Inlined versions for older compiler

static LONG _xInterlockedAnd(volatile xUInt *atomic, xUInt val)
{
	LONG i, j;

	j = *atomic;
	do
	{
		i = j;
		j = InterlockedCompareExchange(atomic, i & val, i);
	} while (i != j);

	return j;
}

#define InterlockedAnd(a, b) _xInterlockedAnd(a, b)

static LONG _xInterlockedOr(volatile xUInt *atomic, xUInt val)
{
	LONG i, j;

	j = *atomic;

	do
	{
		i = j;
		j = InterlockedCompareExchange(atomic, i | val, i);
	} while (i != j);

	return j;
}

#define InterlockedOr(a,b) _xInterlockedOr(a,b)

static LONG _xInterlockedXor(volatile xUInt *atomic, xUInt val)
{
	LONG i, j;

	j = *atomic;
	do
	{
		i = j;
		j = InterlockedCompareExchange(atomic, i ^ val, i);
	} while (i != j);

	return j;
}

#define InterlockedXor(a, b) _xInterlockedXor(a, b)

#endif // !_MSC_VER || _MSC_VER <= 1200

xInt (x_atomic_int_get)(const volatile xInt *atomic)
{
	MemoryBarrier();
	return *atomic;
}

void (x_atomic_int_set)(volatile xInt *atomic, xInt newval)
{
	*atomic = newval;
	MemoryBarrier();
}

void (x_atomic_int_inc)(volatile xInt *atomic)
{
	InterlockedIncrement(atomic);
}

xBoolean (x_atomic_int_dec_and_test)(volatile xInt *atomic)
{
	return InterlockedDecrement(atomic) == 0;
}

xBoolean (x_atomic_int_compare_and_exchange)(volatile xInt *atomic, 
	xInt oldval,
	xInt newval)
{
	return InterlockedCompareExchange(atomic, newval, oldval) == oldval;
}

xInt (x_atomic_int_add)(volatile xInt *atomic, xInt val)
{
	return InterlockedExchangeAdd(atomic, val);
}

xUInt (x_atomic_int_and)(volatile xUInt *atomic, xUInt val)
{
	return InterlockedAnd(atomic, val);
}

xUInt (x_atomic_int_or)(volatile xUInt *atomic, xUInt val)
{
	return InterlockedOr(atomic, val);
}

xUInt (x_atomic_int_xor)(volatile xUInt *atomic, xUInt val)
{
	return InterlockedXor(atomic, val);
}

xPointer (x_atomic_pointer_get)(const volatile void *atomic)
{
	const volatile xPointer *ptr = atomic;
	MemoryBarrier();
	return *ptr;
}

void (x_atomic_pointer_set)(volatile void *atomic, xPointer newval)
{
	volatile xPointer *ptr = atomic;

	*ptr = newval;
	MemoryBarrier();
}

xBoolean (x_atomic_pointer_compare_and_exchange)(volatile void *atomic, 
	xPointer oldval,
	xPointer newval)
{
	return InterlockedCompareExchangePointer(atomic, newval, oldval) == oldval;
}

xSSize (x_atomic_pointer_add)(volatile void *atomic, xSSize val)
{
#ifdef X_OS_WIN64
	return InterlockedExchangeAdd64(atomic, val);
#else
	return InterlockedExchangeAdd(atomic, val);
#endif
}

xSize (x_atomic_pointer_and)(volatile void *atomic, xSize val)
{
#ifdef X_OS_WIN64
	return InterlockedAnd64(atomic, val);
#else
	return InterlockedAnd(atomic, val);
#endif
}

xSize (x_atomic_pointer_or)(volatile void *atomic, xSize val)
{
#ifdef X_OS_WIN64
	return InterlockedOr64(atomic, val);
#else
	return InterlockedOr(atomic, val);
#endif
}

xSize (x_atomic_pointer_xor)(volatile void *atomic, xSize val)
{
#ifdef X_OS_WIN64
	return InterlockedXor64(atomic, val);
#else
	return InterlockedXor(atomic, val);
#endif
}

#else // __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4

#error X_ATOMIC_LOCK_FREE defined, but incapable of lock-free atomics.

#endif // __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4

#else // X_ATOMIC_LOCK_FREE

#include <pthread.h>

static pthread_mutex_t x_atomic_lock = PTHREAD_MUTEX_INITIALIZER;

xInt (x_atomic_int_get)(const volatile xInt *atomic)
{
	xInt value;

	pthread_mutex_lock(&x_atomic_lock);
	value = *atomic;
	pthread_mutex_unlock(&x_atomic_lock);

	return value;
}

void (x_atomic_int_set)(volatile xInt *atomic, xInt value)
{
	pthread_mutex_lock(&x_atomic_lock);
	*atomic = value;
	pthread_mutex_unlock(&x_atomic_lock);
}

void (x_atomic_int_inc)(volatile xInt *atomic)
{
	pthread_mutex_lock(&x_atomic_lock);
	(*atomic)++;
	pthread_mutex_unlock(&x_atomic_lock);
}

xBoolean (x_atomic_int_dec_and_test) (volatile xInt *atomic)
{
	xBoolean is_zero;

	pthread_mutex_lock(&x_atomic_lock);
	is_zero = --(*atomic) == 0;
	pthread_mutex_unlock(&x_atomic_lock);

	return is_zero;
}

xBoolean (x_atomic_int_compare_and_exchange) (volatile xInt *atomic,
	xInt oldval, 
	xInt newval)
{
	xBoolean success;

	pthread_mutex_lock(&x_atomic_lock);

	if ((success = (*atomic == oldval)))
		*atomic = newval;

	pthread_mutex_unlock(&x_atomic_lock);

	return success;
}

xInt (x_atomic_int_add) (volatile xInt *atomic, xInt val)
{
	xInt oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *atomic;
	*atomic = oldval + val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}

xUInt (x_atomic_int_and) (volatile xUInt *atomic, xUInt val)
{
	xUInt oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *atomic;
	*atomic = oldval & val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}

xUInt (x_atomic_int_or) (volatile xUInt *atomic, xUInt val)
{
	xUInt oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *atomic;
	*atomic = oldval | val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}

xUInt (x_atomic_int_xor) (volatile xUInt *atomic, xUInt val)
{
	xUInt oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *atomic;
	*atomic = oldval ^ val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}


xPointer (x_atomic_pointer_get) (const volatile void *atomic)
{
	const volatile xPointer *ptr = atomic;
	xPointer value;

	pthread_mutex_lock(&x_atomic_lock);
	value = *ptr;
	pthread_mutex_unlock(&x_atomic_lock);

	return value;
}

void (x_atomic_pointer_set)(volatile void *atomic, xPointer newval)
{
	volatile xPointer *ptr = atomic;

	pthread_mutex_lock(&x_atomic_lock);
	*ptr = newval;
	pthread_mutex_unlock(&x_atomic_lock);
}

xBoolean (x_atomic_pointer_compare_and_exchange) (volatile void *atomic, 
	xPointer oldval,
	xPointer newval)
{
	volatile xPointer *ptr = atomic;
	xBoolean success;

	pthread_mutex_lock(&x_atomic_lock);

	if ((success = (*ptr == oldval)))
		*ptr = newval;

	pthread_mutex_unlock(&x_atomic_lock);

	return success;
}

xSSize (x_atomic_pointer_add) (volatile void *atomic, 
	xSSize val)
{
	volatile xSSize *ptr = atomic;
	xSSize oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *ptr;
	*ptr = oldval + val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}

xSize (x_atomic_pointer_and) (volatile void *atomic, xSize val)
{
	volatile xSize *ptr = atomic;
	xSize oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *ptr;
	*ptr = oldval & val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}

xSize (x_atomic_pointer_or) (volatile void *atomic, xSize val)
{
	volatile xSize *ptr = atomic;
	xSize oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *ptr;
	*ptr = oldval | val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}

xSize (x_atomic_pointer_xor) (volatile void *atomic, xSize val)
{
	volatile xSize *ptr = atomic;
	xSize oldval;

	pthread_mutex_lock(&x_atomic_lock);
	oldval = *ptr;
	*ptr = oldval ^ val;
	pthread_mutex_unlock(&x_atomic_lock);

	return oldval;
}

#endif // X_ATOMIC_LOCK_FREE

