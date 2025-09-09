// JNI types
#include <jni.h>
// Guard
#ifndef JNISTUFF
#define JNISTUFF

extern JavaVM *jvm;
extern jclass UnityPlayer_cls;
extern jfieldID UnityPlayer_CurrentActivity_fid;
JNIEnv *getEnv();
jobject getGlobalContext(JNIEnv *env);
jobject GetUnitySurface(JNIEnv *env);
void displayKeyboard(bool pShow);

#endif // JNISTUFF
