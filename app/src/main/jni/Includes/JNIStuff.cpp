#include "JNIStuff.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include <jni.h>
#include <stddef.h>

JavaVM *jvm = nullptr;
jclass UnityPlayer_cls = nullptr;
jfieldID UnityPlayer_CurrentActivity_fid = nullptr;

JNIEnv *getEnv()
{
    LOGI(OBFUSCATE("JNIStuff: getEnv called"), 1);
    JNIEnv *env;
    int status = jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (status < 0)
    {
        LOGW(OBFUSCATE("JNIStuff: GetEnv failed, trying AttachCurrentThread"), 1);
        status = jvm->AttachCurrentThread(&env, NULL);
        if (status < 0)
        {
            LOGE(OBFUSCATE("JNIStuff: Error Getting JNI"), 1);
            return nullptr;
        }
    }
    LOGI(OBFUSCATE("JNIStuff: getEnv success"), 1);
    return env;
}

jobject getGlobalContext(JNIEnv *env)
{
    LOGI(OBFUSCATE("getGlobalContext called"), 1);
    jclass activityThread = env->FindClass(OBFUSCATE("android/app/ActivityThread"));
    if (!activityThread)
    {
        LOGE(OBFUSCATE("Failed to find ActivityThread class"), 1);
        return nullptr;
    }
    jmethodID currentActivityThread = env->GetStaticMethodID(activityThread, OBFUSCATE("currentActivityThread"), OBFUSCATE("()Landroid/app/ActivityThread;"));
    if (!currentActivityThread)
    {
        LOGE(OBFUSCATE("Failed to find currentActivityThread method"), 1);
        return nullptr;
    }
    jobject at = env->CallStaticObjectMethod(activityThread, currentActivityThread);
    jmethodID getApplication = env->GetMethodID(activityThread, OBFUSCATE("getApplication"), OBFUSCATE("()Landroid/app/Application;"));
    if (!getApplication)
    {
        LOGE(OBFUSCATE("Failed to find getApplication method"), 1);
        return nullptr;
    }
    jobject context = env->CallObjectMethod(at, getApplication);
    return context;
}

jobject GetUnitySurface(JNIEnv *env)
{
    LOGI(OBFUSCATE("GetUnitySurface: called"), 1);
    LOGI(OBFUSCATE("GetUnitySurface: Getting Unity Activity"), 1);
    jobject activity = env->GetStaticObjectField(UnityPlayer_cls, UnityPlayer_CurrentActivity_fid);
    if (!activity)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to get Unity Activity"), 1);
        return nullptr;
    }
    LOGI(OBFUSCATE("GetUnitySurface: Getting Unity Player Field"), 1);
    jfieldID unityPlayerField = env->GetFieldID(env->GetObjectClass(activity), "mUnityPlayer", "Lcom/unity3d/player/UnityPlayer;");
    if (!unityPlayerField)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to get UnityPlayer field"), 1);
        return nullptr;
    }
    LOGI(OBFUSCATE("GetUnitySurface: Getting Unity Player Object"), 1);
    jobject unityPlayer = env->GetObjectField(activity, unityPlayerField);
    if (!unityPlayer)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to get UnityPlayer object"), 1);
        return nullptr;
    }
    LOGI(OBFUSCATE("GetUnitySurface: Getting View Method"), 1);
    jmethodID getViewMethod = env->GetMethodID(env->GetObjectClass(unityPlayer), "getView", "()Landroid/view/View;");
    if (!getViewMethod)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to get getView method"), 1);
        return nullptr;
    }
    LOGI(OBFUSCATE("GetUnitySurface: Calling getView Method"), 1);
    jobject view = env->CallObjectMethod(unityPlayer, getViewMethod);
    if (!view)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to get View object"), 1);
        return nullptr;
    }
    // Print the actual class name of the view
    jclass viewClass = env->GetObjectClass(view);
    jmethodID getClassMethod = env->GetMethodID(viewClass, "getClass", "()Ljava/lang/Class;");
    jobject classObj = env->CallObjectMethod(view, getClassMethod);
    jclass classClass = env->FindClass("java/lang/Class");
    jmethodID getNameMethod = env->GetMethodID(classClass, "getName", "()Ljava/lang/String;");
    jstring nameStr = (jstring)env->CallObjectMethod(classObj, getNameMethod);
    const char *nameCStr = env->GetStringUTFChars(nameStr, 0);
    LOGI(("GetUnitySurface: View class name: %s"), nameCStr, 1);
    env->ReleaseStringUTFChars(nameStr, nameCStr);

    LOGI("GetUnitySurface: Trying getHolder on UnityPlayer view", 1);
    jmethodID getHolderMethod = env->GetMethodID(viewClass, "getHolder", "()Landroid/view/SurfaceHolder;");
    jobject holder = nullptr;
    if (getHolderMethod)
    {
        holder = env->CallObjectMethod(view, getHolderMethod);
        if (holder)
        {
            LOGI("GetUnitySurface: Got SurfaceHolder from UnityPlayer view", 1);
        }
    }
    if (!holder)
    {
        LOGI("GetUnitySurface: Searching for SurfaceView child", 1);
        jclass viewGroupClass = env->FindClass("android/view/ViewGroup");
        if (env->IsInstanceOf(view, viewGroupClass))
        {
            jmethodID getChildCountMethod = env->GetMethodID(viewGroupClass, "getChildCount", "()I");
            jmethodID getChildAtMethod = env->GetMethodID(viewGroupClass, "getChildAt", "(I)Landroid/view/View;");
            int childCount = env->CallIntMethod(view, getChildCountMethod);
            for (int i = 0; i < childCount; ++i)
            {
                jobject childView = env->CallObjectMethod(view, getChildAtMethod, i);
                jclass childClass = env->GetObjectClass(childView);
                jobject childClassObj = env->CallObjectMethod(childView, getClassMethod);
                jstring childNameStr = (jstring)env->CallObjectMethod(childClassObj, getNameMethod);
                const char *childNameCStr = env->GetStringUTFChars(childNameStr, 0);
                LOGI(("GetUnitySurface: Child %d class name: %s"), i, childNameCStr, 1);
                env->ReleaseStringUTFChars(childNameStr, childNameCStr);
                // Try getHolder on child
                jmethodID childGetHolderMethod = env->GetMethodID(childClass, "getHolder", "()Landroid/view/SurfaceHolder;");
                if (childGetHolderMethod)
                {
                    holder = env->CallObjectMethod(childView, childGetHolderMethod);
                    if (holder)
                    {
                        LOGI(("GetUnitySurface: Got SurfaceHolder from child %d"), i, 1);
                        break;
                    }
                }
            }
        }
        else
        {
            LOGI("GetUnitySurface: UnityPlayer view is not a ViewGroup", 1);
        }
    }
    if (!holder)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to get SurfaceHolder object from UnityPlayer or its children"), 1);
        return nullptr;
    }
    LOGI(OBFUSCATE("GetUnitySurface: Getting Surface from Holder"), 1);
    jclass holderClass = env->FindClass("android/view/SurfaceHolder");
    if (!holderClass)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to find SurfaceHolder class"), 1);
        return nullptr;
    }
    jmethodID getSurfaceMethod = env->GetMethodID(holderClass, "getSurface", "()Landroid/view/Surface;");
    if (!getSurfaceMethod)
    {
        LOGE(OBFUSCATE("GetUnitySurface: Failed to get getSurface method"), 1);
        return nullptr;
    }
    LOGI(OBFUSCATE("GetUnitySurface: Calling getSurface Method"), 1);
    jobject surface = env->CallObjectMethod(holder, getSurfaceMethod);
    LOGI(OBFUSCATE("GetUnitySurface: Successfully obtained Surface"), 1);
    return surface; // This is the Java Surface object
}

void displayKeyboard(bool pShow)
{
    JNIEnv *env = getEnv();
    jclass ctx = env->FindClass(OBFUSCATE("android/content/Context"));
    jfieldID fid = env->GetStaticFieldID(ctx, OBFUSCATE("INPUT_METHOD_SERVICE"), OBFUSCATE("Ljava/lang/String;"));
    jmethodID mid = env->GetMethodID(ctx, OBFUSCATE("getSystemService"), OBFUSCATE("(Ljava/lang/String;)Ljava/lang/Object;"));
    jobject context = env->GetStaticObjectField(UnityPlayer_cls, UnityPlayer_CurrentActivity_fid);
    jobject InputManObj = env->CallObjectMethod(context, mid, (jstring)env->GetStaticObjectField(ctx, fid));
    jclass ClassInputMethodManager = env->FindClass(OBFUSCATE("android/view/inputmethod/InputMethodManager"));
    jmethodID toggleSoftInputId = env->GetMethodID(ClassInputMethodManager, OBFUSCATE("toggleSoftInput"), OBFUSCATE("(II)V"));
    if (pShow)
    {
        env->CallVoidMethod(InputManObj, toggleSoftInputId, 2, 0);
    }
    else
    {
        env->CallVoidMethod(InputManObj, toggleSoftInputId, 0, 0);
    }
}