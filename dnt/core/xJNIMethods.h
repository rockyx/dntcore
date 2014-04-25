#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XJNIMETHODS_H__
#define __DNT_CORE_XJNIMETHODS_H__

#include <dnt/core/xGlobal.h>

#ifdef BUILD_JAVA_NATIVE

#include <jni.h>

X_CORE_BEGIN_DECLS

X_CORE_EXPORT jlong x_get_native_pointer(JNIEnv *env, jobject obj);
X_CORE_EXPORT void x_set_native_pointer(JNIEnv *env, jobject obj, xPointer p);
X_CORE_EXPORT void x_throw_null_pointer_exception(JNIEnv *env, const xChar *msg);
X_CORE_EXPORT void x_throw_io_exception(JNIEnv *env, const xChar *msg);
X_CORE_EXPORT void x_throw_index_out_of_bounds_exception(JNIEnv *env, const xChar *msg);
X_CORE_EXPORT void x_throw_timeout_exception(JNIEnv *env, const xChar *msg);
X_CORE_EXPORT void x_throw_class_cast_exception(JNIEnv *env, const xChar *msg);

X_CORE_END_DECLS

#endif


#endif // __DNT_CORE_XJNIMETHODS_H__
