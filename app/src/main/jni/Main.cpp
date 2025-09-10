#include "Includes/Utils.h"
#include "Includes/obfuscate.h"
#include "Includes/Logger.h"
#include "Includes/JNIStuff.h"
#include "Includes/Loader.h"
#include "Menu/Menu.h"
#include "Includes/Dobby/dobby.h"
#include <pthread.h>
#include <dlfcn.h>

EGLBoolean (*old_eglSwapBuffers)(...);
EGLBoolean hook_eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    eglQuerySurface(dpy, surface, EGL_WIDTH, &glWidth);
    eglQuerySurface(dpy, surface, EGL_HEIGHT, &glHeight);

    if (!setup)
    {
        SetupImGui();
        setup = true;
    }
    Menu::DrawImGuiMenu();
    // RenderImGuiFrame();
    return old_eglSwapBuffers(dpy, surface);
}

extern "C" JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved)
{
    jvm = vm;
    JNIEnv *globalEnv;
    vm->GetEnv((void **)&globalEnv, JNI_VERSION_1_6);
    UnityPlayer_cls = globalEnv->FindClass(OBFUSCATE("com/unity3d/player/UnityPlayer"));
    UnityPlayer_CurrentActivity_fid = globalEnv->GetStaticFieldID(UnityPlayer_cls, OBFUSCATE("currentActivity"), OBFUSCATE("Landroid/app/Activity;"));
    if (DobbyHook((void *)globalEnv->functions->RegisterNatives, (void *)hook_RegisterNatives, (void **)&old_RegisterNatives) == 0)
    {
        LOGI("JNI_OnLoad: Successfully installed RegisterNatives hook");
    }
    else
    {
        LOGE("JNI_OnLoad: Failed to install RegisterNatives hook");
    }

    return JNI_VERSION_1_6;
}

#define targetLibName OBFUSCATE("libil2cpp.so")

void *hack_thread(void *)
{

    LOGI(OBFUSCATE("hack_thread started"));
    int waitCount = 0;
    do
    {
        sleep(1);
        waitCount++;
        if (waitCount % 5 == 0)
        {
            LOGD(OBFUSCATE("Waiting for library: %s (%d seconds)"), (char *)targetLibName, waitCount);
        }
    } while (!isLibraryLoaded((char *)targetLibName));
    LOGI("Library loaded: %s", (char *)targetLibName);
    address = findLibrary((char *)targetLibName);
    LOGI("Library address: 0x%lx", address);
    LOGI(OBFUSCATE("hack_thread finished setup"));
    return NULL;
}

__attribute__((constructor)) void lib_main()
{
    auto eglhandle = dlopen(OBFUSCATE("libEGL.so"), RTLD_LAZY);
    dlerror();
    auto eglSwapBuffers = dlsym(eglhandle, OBFUSCATE("eglSwapBuffers"));
    const char *dlsym_error = dlerror();
    if (dlsym_error)
    {
        LOGE(("lib_main: Cannot load symbol 'eglSwapBuffers': %s"), dlsym_error);
    }
    else
    {
        DobbyHook(eglSwapBuffers, (void *)hook_eglSwapBuffers, (void **)&old_eglSwapBuffers);
    }
    pthread_t ptid;
    int threadResult = pthread_create(&ptid, NULL, hack_thread, NULL);
    if (threadResult == 0)
        LOGI(OBFUSCATE("lib_main: Exit"));
}
