#include "xJNIMethods.h"

#ifdef BUILD_JAVA_NATIVE

jlong x_get_native_pointer(JNIEnv *env, jobject obj)
{
	jclass cls = (*env)->GetObjectClass(env, obj);
	jfieldID ptrId = (*env)->GetFieldID(env, cls, "nativePtr", "J");
	return (*env)->GetLongField(env, obj, ptrId);
}

void x_set_native_pointer(JNIEnv *env, jobject obj, xPointer p)
{
	jclass cls = (*env)->GetObjectClass(env, obj);
	jfieldID ptrId = (*env)->GetFieldID(env, cls, "nativePtr", "J");
	(*env)->SetLongField(env, obj, ptrId, (jlong)p);
}

void x_throw_null_pointer_exception(JNIEnv *env, const xChar *msg)
{
	jclass nullExp;
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	nullExp = (*env)->FindClass(env, "java/lang/NullPointerException");
	if (nullExp == NULL)
	{
		/* Unable to find the exception class, give up */
	}
	else
	{
		(*env)->ThrowNew(env, nullExp, msg);
	}
}

void x_throw_io_exception(JNIEnv *env, const xChar *msg)
{
	jclass ioExp;
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	ioExp = (*env)->FindClass(env, "java/io/IOException");
	if (ioExp == NULL)
	{
		/* Unable to find the exception class, give up */
	}
	else
	{
		(*env)->ThrowNew(env, ioExp, msg);
	}
}

void x_throw_index_out_of_bounds_exception(JNIEnv *env, const xChar *msg)
{
	jclass exp;
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	exp = (*env)->FindClass(env, "java/lang/IndexOutOfBoundsException");
	if (exp == NULL)
	{
		/* Unable to find the exception class, give up */
	}
	else
	{
		(*env)->ThrowNew(env, exp, msg);
	}
}

void x_throw_timeout_exception(JNIEnv *env, const xChar *msg)
{
	jclass exp;
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	exp = (*env)->FindClass(env, "java/util/concurrent/TimeoutException");
	if (exp == NULL)
	{
	}
	else
	{
		(*env)->ThrowNew(env, exp, msg);
	}
}

void x_throw_class_cast_exceptioin(JNIEnv *env, const xChar *msg)
{
	jclass exp;
	(*env)->ExceptionDescribe(env);
	(*env)->ExceptionClear(env);
	exp = (*env)->FindClass(env, "java/lang/ClassCastException");
	if (exp == NULL)
	{
	}
	else
	{
		(*env)->ThrowNew(env, exp, msg);
	}
}

#endif
