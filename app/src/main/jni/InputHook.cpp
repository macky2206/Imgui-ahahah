#include "Includes/Loader.h"
#include "Includes/Logger.h"
#include <mutex>
#include <queue>
#include <string>
#include "ImGui/imgui.h"

static std::mutex g_input_mutex;
static std::queue<std::string> g_input_queue;

void DrainQueuedInputToImGui()
{
    std::lock_guard<std::mutex> lk(g_input_mutex);
    ImGuiIO &io = ImGui::GetIO();
    int drained = 0;
    while (!g_input_queue.empty())
    {
        std::string s = g_input_queue.front();
        g_input_queue.pop();
        // Special token for backspace
        if (s == "\b")
        {
            // Simulate a backspace key press (down then up)
            io.AddKeyEvent(ImGuiKey_Backspace, true);
            io.AddKeyEvent(ImGuiKey_Backspace, false);
        }
        else
        {
            io.AddInputCharactersUTF8(s.c_str());
        }
        drained++;
    }
    if (drained > 0)
        LOGI("Drained %d IME input(s) into ImGui", drained);
}

const char *getString(uintptr_t offset)
{
    static std::string convert;
    convert = std::to_string(offset);
    return convert.c_str();
}

uintptr_t address = 0;
bool setup = false;

TOUCH_EVENT g_LastTouchEvent = {TOUCH_ACTION_UP, 0.0f, 0.0f, 0, 0.0f, 0.0f};

bool HandleInputEvent(JNIEnv *env, int motionEvent, int x, int y, int p)
{
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

jboolean (*old_nativeInjectEvent)(JNIEnv *env, jobject instance, jobject event) = nullptr;
jboolean hook_nativeInjectEvent(JNIEnv *env, jobject instance, jobject event)
{
    jclass MotionEvent = env->FindClass("android/view/MotionEvent");
    if (MotionEvent && env->IsInstanceOf(event, MotionEvent))
    {
        jmethodID id_getAct = env->GetMethodID(MotionEvent, "getActionMasked", "()I");
        jmethodID id_getX = env->GetMethodID(MotionEvent, "getX", "()F");
        jmethodID id_getY = env->GetMethodID(MotionEvent, "getY", "()F");
        jmethodID id_getPs = env->GetMethodID(MotionEvent, "getPointerCount", "()I");
        int action = id_getAct ? env->CallIntMethod(event, id_getAct) : -1;
        int x = id_getX ? (int)env->CallFloatMethod(event, id_getX) : 0;
        int y = id_getY ? (int)env->CallFloatMethod(event, id_getY) : 0;
        int pointers = id_getPs ? env->CallIntMethod(event, id_getPs) : 0;
        LOGD("Processing Touch Event: action=%d x=%d y=%d pointers=%d", action, x, y, pointers);
        HandleInputEvent(env, action, x, y, pointers);
        LOGI("Touch Event Processed (action=%d)", action);
        if (!ImGui::GetIO().MouseDownOwnedUnlessPopupClose[0])
        {
            return old_nativeInjectEvent ? old_nativeInjectEvent(env, instance, event) : JNI_FALSE;
        }
        return JNI_TRUE;
    }

    // Detect KeyEvent and try to extract committed characters
    jclass KeyEvent = env->FindClass("android/view/KeyEvent");
    if (KeyEvent && env->IsInstanceOf(event, KeyEvent))
    {
        LOGD("Processing Key Event");
        jmethodID id_getAction = env->GetMethodID(KeyEvent, "getAction", "()I");
        jmethodID id_getKeyCode = env->GetMethodID(KeyEvent, "getKeyCode", "()I");
        jmethodID id_getUnicodeChar = env->GetMethodID(KeyEvent, "getUnicodeChar", "()I");
        jmethodID id_getCharacters = env->GetMethodID(KeyEvent, "getCharacters", "()Ljava/lang/String;");

        int action = id_getAction ? env->CallIntMethod(event, id_getAction) : -1;
        int keycode = id_getKeyCode ? env->CallIntMethod(event, id_getKeyCode) : -1;
        int uni = id_getUnicodeChar ? env->CallIntMethod(event, id_getUnicodeChar) : 0;
        LOGD("KeyEvent: action=%d keycode=%d unicode=%d", action, keycode, uni);

        jstring jchars = NULL;
        const char *chars = NULL;
        if (action == 2 && id_getCharacters && (keycode != 67)) // Exclude DEL key
        {
            jchars = (jstring)env->CallObjectMethod(event, id_getCharacters);
            if (jchars)
                chars = env->GetStringUTFChars(jchars, nullptr);
        }

        if (action == 2 && chars && chars[0] != '\0') // Only enqueue for ACTION_MULTIPLE
        {
            std::lock_guard<std::mutex> lk(g_input_mutex);
            g_input_queue.push(std::string(chars));
            LOGI("hook_nativeInjectEvent queued ACTION_MULTIPLE text: %.128s", chars);
            env->ReleaseStringUTFChars(jchars, chars);
        }
        else if (uni != 0)
        {
            // Convert unicode code point to UTF-8
            char buf[5] = {0};
            if (uni < 0x80)
            {
                buf[0] = (char)uni;
            }
            else if (uni < 0x800)
            {
                buf[0] = (char)(0xC0 | ((uni >> 6) & 0x1F));
                buf[1] = (char)(0x80 | (uni & 0x3F));
            }
            else if (uni < 0x10000)
            {
                buf[0] = (char)(0xE0 | ((uni >> 12) & 0x0F));
                buf[1] = (char)(0x80 | ((uni >> 6) & 0x3F));
                buf[2] = (char)(0x80 | (uni & 0x3F));
            }
            else
            {
                buf[0] = (char)(0xF0 | ((uni >> 18) & 0x07));
                buf[1] = (char)(0x80 | ((uni >> 12) & 0x3F));
                buf[2] = (char)(0x80 | ((uni >> 6) & 0x3F));
                buf[3] = (char)(0x80 | (uni & 0x3F));
            }
            std::lock_guard<std::mutex> lk(g_input_mutex);
            g_input_queue.push(std::string(buf));
            LOGI("hook_nativeInjectEvent queued unicode char: %s", buf);
        }
        else if (action == 0 && keycode == 67) // ACTION_DOWN and DEL key
        {
            // Backspace (KEYCODE_DEL)
            std::lock_guard<std::mutex> lk(g_input_mutex);
            g_input_queue.push(std::string("\b"));
            LOGI("hook_nativeInjectEvent queued backspace token");
        }

        // Forward KeyEvent to original implementation
        return old_nativeInjectEvent ? old_nativeInjectEvent(env, instance, event) : JNI_FALSE;
    }

    // Not MotionEvent or KeyEvent -> forward
    return old_nativeInjectEvent ? old_nativeInjectEvent(env, instance, event) : JNI_FALSE;
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
    for (int i = 0; i < totalMethodCount; ++i)
    {
        if (!strcmp(methods[i].name, ("nativeInjectEvent")))
        {
            LOGI("hook_RegisterNatives: hooking nativeInjectEvent");
            DobbyHook(methods[i].fnPtr, (void *)hook_nativeInjectEvent, (void **)&old_nativeInjectEvent);
        }
    }
    return old_RegisterNatives(env, destinationClass, methods, totalMethodCount);
}
