#include <EGL/egl.h>
#include <jni.h>
#include <GLES2/gl2.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "Dobby/dobby.h"
#include <mutex>
#include <queue>

// Converts an offset to a string and returns a pointer to a static buffer
#include <string>
using std::string;
const char *getString(uintptr_t offset);

extern uintptr_t address;
extern bool setup;
bool HandleInputEvent(JNIEnv *env, int motionEvent, int x, int y, int p);
typedef enum
{
    TOUCH_ACTION_DOWN = 0,
    TOUCH_ACTION_UP,
    TOUCH_ACTION_MOVE
} TOUCH_ACTION;
typedef struct
{
    TOUCH_ACTION action;
    float x;
    float y;
    int pointers;
    float y_velocity;
    float x_velocity;
} TOUCH_EVENT;
extern TOUCH_EVENT g_LastTouchEvent;

// Drain queued IME input into ImGui (implemented in InputHook.cpp)
void DrainQueuedInputToImGui();

// Hooks and globals declared; implementations are in InputHook.cpp
extern jboolean (*old_nativeInjectEvent)(JNIEnv *env, jobject instance, jobject event);
extern void (*old_nativeSetInputString)(JNIEnv *env, jobject instance, jstring str);
extern jint (*old_RegisterNatives)(JNIEnv *, jclass, JNINativeMethod *, jint);

// Hook functions implemented in InputHook.cpp
jboolean hook_nativeInjectEvent(JNIEnv *env, jobject instance, jobject event);
void hook_nativeSetInputString(JNIEnv *env, jobject instance, jstring str);
jint hook_RegisterNatives(JNIEnv *env, jclass destinationClass, JNINativeMethod *methods, jint totalMethodCount);
