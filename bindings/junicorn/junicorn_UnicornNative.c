#include <sys/types.h>
#include "unicorn/platform.h"
#include "uc_priv.h"
#include <stdlib.h>
#include <string.h>
#include <unicorn/unicorn.h>
#include <unicorn/x86.h>
#include "junicorn_UnicornNative.h"

static jclass class_juc;
static jclass class_juc_exception;
static jclass class_juc_memoryRegion;
static jmethodID method_juc_error;
static jmethodID method_juc_hook_cb;
static jmethodID method_juc_exception_init;
static jmethodID method_juc_memoryRegion_init;
static jfieldID fieldEngine;

static JavaVM* cachedJVM;

typedef enum juc_hook_cb_type {
	JUC_HOOK_CB_CODE = 1 << 0,
	JUC_HOOK_CB_MEM_INVAILD = 1 << 1,
	JUC_HOOK_CB_MEM_ACCESS = 1 << 2,
	JUC_HOOK_CB_INTR_CB = 1 << 3,
	JUC_HOOK_CB_INSN_IN = 1 << 4,
	JUC_HOOK_CB_INSN_OUT = 1 << 5,
	JUC_HOOK_CB_INSN_SYSCALL = 1 << 6,
} juc_hook_cb_type;

static void juc_throw(JNIEnv *env, uc_err code) {
	const char *message = uc_strerror(code);
	jobject exc = (*env)->NewObject(env, class_juc_exception, method_juc_exception_init, (jint)code);
	(*env)->Throw(env, exc);
}

static jint juc_hook_cb(JavaVM *jvm, uc_engine *engine, void *user_data, jlong arg1, jlong arg2, jlong arg3, jlong arg4)
{
	JNIEnv *env;
	(*jvm)->AttachCurrentThread(jvm, (void **)&env, NULL);
	jint result = (*env)->CallStaticIntMethod(
		env,
		class_juc,
		method_juc_hook_cb,
		(jlong)engine,
		(jlong)user_data,
		(jlong)arg1,
		(jlong)arg2,
		(jlong)arg3,
		(jlong)arg4
	);
	(*cachedJVM)->DetachCurrentThread(cachedJVM);
	return result;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
	JNIEnv *env;

	jint result = (*jvm)->GetEnv(jvm, (void **)&env, JNI_VERSION_1_6);
	if (result != JNI_OK)
		return JNI_VERSION_1_6;

	cachedJVM = jvm;
	class_juc = (*env)->FindClass(env, "junicorn/UnicornNative");
	class_juc_exception = (*env)->FindClass(env, "junicorn/UnicornException");
	class_juc_memoryRegion = (*env)->FindClass(env, "junicorn/MemoryRegion");
	method_juc_hook_cb = (*env)->GetStaticMethodID(env, class_juc, "juc_hook_cb", "(JJJJJJ)I");
	method_juc_exception_init = (*env)->GetMethodID(env, class_juc_exception, "<init>", "(I)V");
	method_juc_memoryRegion_init = (*env)->GetMethodID(env, class_juc_memoryRegion, "<init>", "(JJI)V");
	return JNI_VERSION_1_6;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_version
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_junicorn_UnicornNative_uc_1version
(JNIEnv *env, jclass cls)
{
	return uc_version(NULL, NULL);
}

/*
* Class:     junicorn_UnicornNative
* Method:    uc_arch_supported
* Signature: (I)Z
*/
JNIEXPORT jboolean JNICALL Java_junicorn_UnicornNative_uc_1arch_1supported
(JNIEnv *env, jclass cls, jint arch) {
	return uc_arch_supported((uc_arch)arch);
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_open
 * Signature: (II)J
 */
JNIEXPORT jlong JNICALL Java_junicorn_UnicornNative_uc_1open
(JNIEnv *env, jclass cls, jint arch, jint mode) {
	uc_engine *uc = NULL;
	uc_err code = uc_open(arch, mode, &uc);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}

	return (jlong)uc;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_close
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1close
(JNIEnv *env, jclass cls, jlong engine) {
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_close(uc);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_strerror
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_junicorn_UnicornNative_uc_1strerror(JNIEnv *env, jclass cls, jint code)
{
	const char *message = uc_strerror((uc_err)code);
	jobject obj = (*env)->NewStringUTF(env, message);
	return obj;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_errno
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_junicorn_UnicornNative_uc_1errno
(JNIEnv *env, jclass cls, jlong engine)
{
	uc_engine *uc = (uc_engine *)engine;
	return uc_errno(uc);
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_reg_read
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_junicorn_UnicornNative_uc_1reg_1read__JI
(JNIEnv *env, jclass cls, jlong engine, jint regid)
{
	long value = 0;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_reg_read(uc, (int)regid, (void *)&value);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
		return 0;
	}

	return (jlong)value;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_reg_read
 * Signature: (JII)J
 */
JNIEXPORT jlong JNICALL Java_junicorn_UnicornNative_uc_1reg_1read__JII
(JNIEnv *env, jclass cls, jlong engine, jint regid, jint opt)
{
	juc_throw(env, UC_ERR_OK);
	return 0;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_reg_write
 * Signature: (JIJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1reg_1write
(JNIEnv *env, jclass cls, jlong engine, jint regid, jlong val)
{
	long value = val;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_reg_write(uc, (int)regid, (void *)&value);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_mem_read
 * Signature: (JJ[BJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1mem_1read
(JNIEnv *env, jclass cls, jlong engine, jlong address, jbyteArray data, jlong size)
{
	jbyte *bytes = (*env)->GetByteArrayElements(env, data, NULL);
	jsize arraySize = (*env)->GetArrayLength(env, data);
	size_t memSize = min(arraySize, size);
 	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_mem_read(uc, (uint64_t)address, (void *)bytes, memSize);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
	(*env)->SetByteArrayRegion(env, data, 0, memSize, bytes);
	(*env)->ReleaseByteArrayElements(env, data, bytes, JNI_ABORT);
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_mem_write
 * Signature: (JJ[BJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1mem_1write
(JNIEnv *env, jclass cls, jlong engine, jlong address, jbyteArray data, jlong size)
{
	jbyte *bytes = (*env)->GetByteArrayElements(env, data, NULL);
	jsize arraySize = (*env)->GetArrayLength(env, data);
	size_t memSize = min(arraySize, size);
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_mem_write(uc, (uint64_t)address, (const void *)bytes, memSize);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
	(*env)->ReleaseByteArrayElements(env, data, bytes, JNI_ABORT);
}
/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_emu_start
 * Signature: (JJJJJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1emu_1start
(JNIEnv *env, jclass cls, jlong engine, jlong begin, jlong until, jlong timeout, jlong count)
{
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_emu_start(
		uc,
		(uint64_t)begin, 
		(uint64_t)until,
		(uint64_t)timeout,
		(size_t)count
	);

	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_emu_stop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1emu_1stop
(JNIEnv *env, jclass cls, jlong engine)
{
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_emu_stop(uc);

	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

static void juc_hook_code_cb(uc_engine *engine, uint64_t address, uint32_t size, void *user_data)
{
	juc_hook_cb(cachedJVM, engine, user_data, address, size, 0, 0);
}

static bool juc_hook_mem_invaild_cb(uc_engine *engine, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data)
{
	jint result = juc_hook_cb(cachedJVM, engine, user_data, type, address, size, value);
	return result > 0 ? true : false;
}

static void juc_hook_mem_access_cb(uc_engine *engine, uc_mem_type type, uint64_t address, int size, int64_t value, void *user_data)
{
	juc_hook_cb(cachedJVM, engine, user_data, type, address, size, value);
}

static void juc_hook_intr_cb(uc_engine *engine, uint32_t intno, void *user_data)
{
	juc_hook_cb(cachedJVM, engine, user_data, intno, 0, 0, 0);
}

static uint32_t juc_hook_insn_in_cb(uc_engine *engine, uint32_t port, int size, void *user_data)
{
	jint result = juc_hook_cb(cachedJVM, engine, user_data, port, size, 0, 0);
	return (uint32_t)result;
}

static void juc_hook_insn_out_cb(uc_engine *engine, uint32_t port, int size, uint32_t value, void *user_data)
{
	juc_hook_cb(cachedJVM, engine, user_data, port, size, value, 0);
}

static void juc_hook_insn_syscall_cb(uc_engine *engine, void *user_data)
{
	juc_hook_cb(cachedJVM, engine, user_data, 0, 0, 0, 0);
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_hook_add
 * Signature: (JIIJJJJ)J
 */
JNIEXPORT jlong JNICALL Java_junicorn_UnicornNative_uc_1hook_1add
(JNIEnv *env, jclass cls, jlong engine, jint cb_type, jint type, jlong id, jlong begin, jlong end, jlong arg1)
{
	uc_hook hook = 0;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code;
	
	juc_hook_cb_type cbt = (juc_hook_cb_type)cb_type;
	switch (cbt)
	{
	case JUC_HOOK_CB_CODE:
		code = uc_hook_add(uc, &hook, (int)type, juc_hook_code_cb, (void *)id, (uint64_t)begin, (uint64_t)end);
		break;
	case JUC_HOOK_CB_MEM_INVAILD:
		code = uc_hook_add(uc, &hook, (int)type, juc_hook_mem_invaild_cb, (void *)id, (uint64_t)begin, (uint64_t)end);
		break;
	case JUC_HOOK_CB_MEM_ACCESS:
		code = uc_hook_add(uc, &hook, (int)type, juc_hook_mem_access_cb, (void *)id, (uint64_t)begin, (uint64_t)end);
		break;
	case JUC_HOOK_CB_INTR_CB:
		code = uc_hook_add(uc, &hook, (int)type, juc_hook_intr_cb, (void *)id, (uint64_t)begin, (uint64_t)end);
		break;
	case JUC_HOOK_CB_INSN_IN:
		code = uc_hook_add(uc, &hook, (int)type, juc_hook_insn_in_cb, (void *)id, (uint64_t)begin, (uint64_t)end, (int)arg1);
		break;
	case JUC_HOOK_CB_INSN_OUT:
		code = uc_hook_add(uc, &hook, (int)type, juc_hook_insn_out_cb, (void *)id, (uint64_t)begin, (uint64_t)end, (int)arg1);
		break;
	case JUC_HOOK_CB_INSN_SYSCALL:
		code = uc_hook_add(uc, &hook, (int)type, juc_hook_insn_syscall_cb, (void *)id, (uint64_t)begin, (uint64_t)end, (int)arg1);
		break;
	default:
		code = UC_ERR_OK;
		juc_throw(env, code);
	}

	if (code != UC_ERR_OK) {
		juc_throw(env, code);
		return 0;
	}

	return (jlong)hook;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_hook_del
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1hook_1del
(JNIEnv *env, jclass cls, jlong engine, jlong id)
{
	uc_hook hook = (uc_hook)id;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_hook_del(uc, hook);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_mem_map
 * Signature: (JJJI)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1mem_1map
(JNIEnv *env, jclass cls, jlong engine, jlong address, jlong size, jint perms)
{
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_mem_map(uc, (uint64_t)address, (size_t)size, (uint32_t)perms);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_mem_map_ptr
 * Signature: (JJJIJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1mem_1map_1ptr
(JNIEnv *env, jclass cls, jlong engine, jlong address, jlong size, jint perms, jlong ptr)
{
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_mem_map_ptr(uc, (uint64_t)address, (size_t)size, (uint32_t)perms, (void *)ptr);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_mem_unmap
 * Signature: (JJJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1mem_1unmap
(JNIEnv *env, jclass cls, jlong engine, jlong address, jlong size)
{
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_mem_unmap(uc, (uint64_t)address, (size_t)size);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_mem_protect
 * Signature: (JJJI)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1mem_1protect
(JNIEnv *env, jclass cls, jlong engine, jlong address, jlong size, jint perms)
{
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_mem_protect(uc, (uint64_t)address, (size_t)size, (uint32_t)perms);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_query
 * Signature: (JI)J
 */
JNIEXPORT jlong JNICALL Java_junicorn_UnicornNative_uc_1query
  (JNIEnv *env, jclass cls, jlong engine, jint type)
{
	size_t result = 0;
	uc_query_type qtype = (uc_query_type)type;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_query(uc, qtype, &result);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
		return 0;
	}

	return (jlong)result;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_context_alloc
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_junicorn_UnicornNative_uc_1context_1alloc
(JNIEnv *env, jclass cls, jlong engine)
{
	uc_context *ctx = NULL;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_context_alloc(uc, &ctx);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
		return 0;
	}

	return (jlong)ctx;
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_free
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1free
(JNIEnv *env, jclass cls, jlong context)
{
	uc_context *ctx = (uc_context *)context;
	uc_err code = uc_free(&ctx);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_context_save
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1context_1save
  (JNIEnv *env, jclass cls, jlong engine, jlong context)
{
	uc_context *ctx = (uc_context *)context;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_context_save(uc, ctx);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_context_restore
 * Signature: (JJ)V
 */
JNIEXPORT void JNICALL Java_junicorn_UnicornNative_uc_1context_1restore
  (JNIEnv *env, jclass cls, jlong engine, jlong context)
{
	uc_context *ctx = (uc_context *)context;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_context_restore(uc, ctx);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
	}
}

/*
 * Class:     junicorn_UnicornNative
 * Method:    uc_mem_regions
 * Signature: (J)[Ljunicorn/MemoryRegion;
 */
JNIEXPORT jobjectArray JNICALL Java_junicorn_UnicornNative_uc_1mem_1regions
(JNIEnv *env, jclass cls, jlong engine)
{
	uc_mem_region *regions;
	uint32_t count = 0;
	uc_engine *uc = (uc_engine *)engine;
	uc_err code = uc_mem_regions(uc, &regions, &count);
	if (code != UC_ERR_OK) {
		juc_throw(env, code);
		return NULL;
	}

	jobjectArray arr = (*env)->NewObjectArray(env, (jsize)count, class_juc_memoryRegion, NULL);

	for (int i = 0; i < count; i++) {
		uc_mem_region *region = &regions[i];
		jobject mem = (*env)->NewObject(
			env,
			class_juc_memoryRegion,
			method_juc_memoryRegion_init,
			(jlong)region->begin,
			(jlong)region->end,
			(jint)region->perms
		);

		(*env)->SetObjectArrayElement(env, arr, (jsize)i, mem);
	}

	uc_free(regions);
	return arr;
}

