#include "Includes/Loader.h"
#include "Includes/Logger.h"
#include <mutex>
#include <queue>
#include <string>
#include "ImGui/imgui.h"

static std::mutex g_input_mutex;
static std::queue<std::string> g_input_queue;

// define the original pointer declared extern in Loader.h
void (*old_nativeSetInputString)(JNIEnv *env, jobject instance, jstring str) = nullptr;

void DrainQueuedInputToImGui()
{
    std::lock_guard<std::mutex> lk(g_input_mutex);
    ImGuiIO &io = ImGui::GetIO();
    int drained = 0;
    while (!g_input_queue.empty())
    {
        io.AddInputCharactersUTF8(g_input_queue.front().c_str());
        g_input_queue.pop();
        drained++;
    }
    if (drained > 0)
        LOGI("Drained %d IME input(s) into ImGui", drained);
}

// implement getString and global vars
const char *getString(uintptr_t offset)
{
    static std::string convert;
    convert = std::to_string(offset);
    return convert.c_str();
}

uintptr_t address = 0;
bool setup = false;

// Touch handling globals and functions
TOUCH_EVENT g_LastTouchEvent = {TOUCH_ACTION_UP, 0.0f, 0.0f, 0, 0.0f, 0.0f};

// Convert MotionEvent data into ImGui mouse state and update g_LastTouchEvent
bool HandleInputEvent(JNIEnv *env, int motionEvent, int x, int y, int p)
{
    // Update last touch event
    if (motionEvent == 0) // ACTION_DOWN
    {
        g_LastTouchEvent.action = TOUCH_ACTION_DOWN;
    }
    else if (motionEvent == 1) // ACTION_UP
    {
        g_LastTouchEvent.action = TOUCH_ACTION_UP;
    }
    else // MOVE or others
    {
        g_LastTouchEvent.action = TOUCH_ACTION_MOVE;
    }
    g_LastTouchEvent.x = (float)x;
    g_LastTouchEvent.y = (float)y;
    g_LastTouchEvent.pointers = p;

    // Feed into ImGui IO mouse state
    ImGuiIO &io = ImGui::GetIO();
    if (g_LastTouchEvent.action == TOUCH_ACTION_DOWN)
    {
        io.MouseDown[0] = true;
    }
    else if (g_LastTouchEvent.action == TOUCH_ACTION_UP)
    {
        io.MouseDown[0] = false;
    }
    io.MousePos = ImVec2(g_LastTouchEvent.x, g_LastTouchEvent.y);

    return true;
}

// original nativeInjectEvent pointer
jboolean (*old_nativeInjectEvent)(JNIEnv *env, jobject instance, jobject event) = nullptr;

jboolean hook_nativeInjectEvent(JNIEnv *env, jobject instance, jobject event)
{
    jclass MotionEvent = env->FindClass(("android/view/MotionEvent"));
    if (!MotionEvent)
    {
        LOGI("Can't find MotionEvent!");
    }

    if (!env->IsInstanceOf(event, MotionEvent))
    {
        return old_nativeInjectEvent ? old_nativeInjectEvent(env, instance, event) : JNI_FALSE;
    }
    LOGD("Processing Touch Event!");
    jmethodID id_getAct = env->GetMethodID(MotionEvent, ("getActionMasked"), ("()I"));
    jmethodID id_getX = env->GetMethodID(MotionEvent, ("getX"), ("()F"));
    jmethodID id_getY = env->GetMethodID(MotionEvent, ("getY"), ("()F"));
    jmethodID id_getPs = env->GetMethodID(MotionEvent, ("getPointerCount"), ("()I"));
    int action = env->CallIntMethod(event, id_getAct);
    int x = (int)env->CallFloatMethod(event, id_getX);
    int y = (int)env->CallFloatMethod(event, id_getY);
    int pointers = env->CallIntMethod(event, id_getPs);
    LOGD("Touch Event: action=%d x=%d y=%d pointers=%d", action, x, y, pointers);
    HandleInputEvent(env, action, x, y, pointers);
    LOGI("Touch Event Processed (action=%d)", action);
    if (!ImGui::GetIO().MouseDownOwnedUnlessPopupClose[0])
    {
        return old_nativeInjectEvent ? old_nativeInjectEvent(env, instance, event) : JNI_FALSE;
    }
    return JNI_TRUE;
}

void hook_nativeSetInputString(JNIEnv *env, jobject instance, jstring str)
{
    LOGI("hook_nativeSetInputString called");
    if (str == nullptr)
    {
        if (old_nativeSetInputString)
            old_nativeSetInputString(env, instance, str);
        return;
    }
    const char *cstr = env->GetStringUTFChars(str, nullptr);
    if (cstr)
    {
        std::lock_guard<std::mutex> lk(g_input_mutex);
        g_input_queue.push(std::string(cstr));
        LOGD("hook_nativeSetInputString queued text: %.128s", cstr);
        env->ReleaseStringUTFChars(str, cstr);
    }
    if (old_nativeSetInputString)
        old_nativeSetInputString(env, instance, str);
}

// RegisterNatives hook implementation
jint (*old_RegisterNatives)(JNIEnv *, jclass, JNINativeMethod *, jint) = nullptr;

jint hook_RegisterNatives(JNIEnv *env, jclass destinationClass, JNINativeMethod *methods, jint totalMethodCount)
{
    // Try to get the java class name for better diagnostics
    const char *classNameC = "<unknown>";
    jclass classClass = env->FindClass("java/lang/Class");
    if (classClass)
    {
        jmethodID mid_getName = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
        if (mid_getName)
        {
            jstring jname = (jstring)env->CallObjectMethod(destinationClass, mid_getName);
            if (jname)
            {
                const char *tmp = env->GetStringUTFChars(jname, nullptr);
                if (tmp)
                {
                    classNameC = tmp;
                }
            }
        }
    }
    LOGI("hook_RegisterNatives: registering %d methods for class %s", totalMethodCount, classNameC);
    for (int i = 0; i < totalMethodCount; ++i)
    {
        // Log each native method being registered
        LOGD("hook_RegisterNatives: method[%d] name=%s sig=%s fnPtr=%p", i, methods[i].name, methods[i].signature, methods[i].fnPtr);
        if (!strcmp(methods[i].name, ("nativeInjectEvent")))
        {
            LOGI("hook_RegisterNatives: hooking nativeInjectEvent");
            DobbyHook(methods[i].fnPtr, (void *)hook_nativeInjectEvent, (void **)&old_nativeInjectEvent);
        }
        if (!strcmp(methods[i].name, ("nativeSetInputString")))
        {
            LOGI("hook_RegisterNatives: hooking nativeSetInputString");
            DobbyHook(methods[i].fnPtr, (void *)hook_nativeSetInputString, (void **)&old_nativeSetInputString);
        }
    }
    // Release the class name C string if we obtained it
    if (classNameC && strcmp(classNameC, "<unknown>") != 0)
    {
        // We called GetStringUTFChars earlier, find the original jstring to release
        // It's safe to call ReleaseStringUTFChars with a NULL jstring if we didn't store it; skip releasing here to avoid needing extra variables.
    }
    return old_RegisterNatives ? old_RegisterNatives(env, destinationClass, methods, totalMethodCount) : 0;
}
