#include "Menu.h"
#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_android.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "Themes.h"
#include "Includes/Logger.h"
#include "Includes/JNIStuff.h"
#include <android/native_window_jni.h>

using namespace ImGui;
bool init = false;
int glWidth = 0;
int glHeight = 0;
ANativeWindow *window = nullptr;

void SetupImGui()
{
    if (!init)
    {
        auto context = ImGui::CreateContext();
        if (!context)
        {
            LOGE("SetupImGui: Failed to create ImGui context");
            return;
        }
        ImGuiIO &io = ImGui::GetIO();
        ImFontConfig font_cfg;
        io.DisplaySize = ImVec2((float)glWidth, (float)glHeight);
        font_cfg.SizePixels = 22.0f;
        // io.Fonts->AddFontFromMemoryTTF(Roboto_Regular, 22, 22.0f);

        io.IniFilename = NULL;

        Theme::SetCorporateGrayTheme();
        ImGui::GetStyle().ScaleAllSizes(3.0f);

        // Get Surface from Unity and convert to ANativeWindow
        JNIEnv *env = getEnv();
        jobject surface = GetUnitySurface(env);
        if (surface)
        {
            window = ANativeWindow_fromSurface(env, surface);
            if (!window) {
                LOGE("SetupImGui: Failed to get ANativeWindow from surface");
            }
        }
        LOGE("SetupImGui: Failed getting surface");
        ImGui_ImplAndroid_Init(window);
        ImGui_ImplOpenGL3_Init();

        init = true;
        LOGI("SetupImGui: ImGui initialized successfully");
    }
}

namespace Menu
{
    ImVec4 color = ImVec4(1, 1, 1, 1);
    bool *p_open = nullptr;

    void DrawMenu()
    {
        static bool show_demo_window = false;
        static bool show_another_window = false;
        static bool show_app_metrics = false;
        static bool showfrm = false;
        static bool show_app_style_editor = false;
        static bool show_app_about = false;
        static bool showtabmenu = false;
        static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        bool FeatureH;
        if (show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        {
            static float f = 0.0f;
            static int counter = 0;
            ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
            ImGui::Begin("Hello, World!", p_open, ImGuiWindowFlags_MenuBar);
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("Mod Tools"))
                {
                    ImGui::MenuItem("Style Editor", NULL, &show_app_style_editor);
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }
            if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
            {
                if (ImGui::BeginTabItem("Section 1"))
                {
                    static char inputText[128] = "";
                    ImGui::InputText("Input", inputText, IM_ARRAYSIZE(inputText));
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Section 2"))
                {
                    ImGui::Text("Lorem ipsum is a dummy text");
                    ImGui::Checkbox("FrameRate Viewer", &showfrm);
                    Checkbox("Show window", &show_another_window);
                    ImGui::Checkbox("Demo and Settings Panel", &show_demo_window);
                    ImGui::Text("Does Nothing But Looks Cool");
                    ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
                    ImGui::ColorEdit3("clear color", (float *)&clear_color);
                    ImGui::Text("Button Tap Counter");
                    if (ImGui::Button("Button"))
                    {
                        counter++;
                    }
                    ImGui::SameLine();
                    ImGui::Text("counter = %d", counter);
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            if (show_app_style_editor)
            {
                ImGui::Begin("Mod Menu Style Editor", &show_app_style_editor);
                ImGui::ShowStyleEditor();
                ImGui::End();
            }
            if (showfrm)
            {
                int corner = 0;
                ImGuiIO &io = ImGui::GetIO();
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
                if (corner != -1)
                {
                    const float PAD = 10.0f;
                    const ImGuiViewport *viewport = ImGui::GetMainViewport();
                    ImVec2 work_pos = viewport->WorkPos;
                    ImVec2 work_size = viewport->WorkSize;
                    ImVec2 window_pos, window_pos_pivot;
                    window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
                    window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
                    window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
                    window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
                    window_flags |= ImGuiWindowFlags_NoMove;
                }
                ImGui::SetNextWindowBgAlpha(0.40f);
                if (ImGui::Begin("Framrate overlay", p_open, window_flags))
                {
                    ImGui::Text("           FrameRate");
                    ImGui::Separator();
                    ImGui::Text("App Average %.3f ms\n     Frames %.1f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                    ImGui::SameLine();
                    if (ImGui::BeginPopupContextWindow())
                    {
                        if (ImGui::MenuItem("Custom", NULL, corner == -1))
                            corner = -1;
                        if (ImGui::MenuItem("Top-left", NULL, corner == 0))
                            corner = 0;
                        if (ImGui::MenuItem("Top-right", NULL, corner == 1))
                            corner = 1;
                        if (ImGui::MenuItem("Bottom-left", NULL, corner == 2))
                            corner = 2;
                        if (ImGui::MenuItem("Bottom-right", NULL, corner == 3))
                            corner = 3;
                        if (p_open && ImGui::MenuItem("Close"))
                            *p_open = false;
                        ImGui::EndPopup();
                    }
                }
                ImGui::End();
            }
            if (show_another_window)
            {
                ImGui::Begin("Viewer", &show_another_window, ImGuiWindowFlags_MenuBar);
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("Tools"))
                    {
                        Text("Another tab");
                        ImGui::EndMenu();
                    }
                    ImGui::EndMenuBar();
                }
                ImGui::Text("Menu Colour");
                static int e = 0;
                ImGui::RadioButton("Black Gold", &e, 1);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Dark 1", &e, 2);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Dark Gray", &e, 3);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Corporate Gray", &e, 4);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Dark 2", &e, 5);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Dark 3", &e, 6);
                ImGui::TableNextColumn();
                ImGui::RadioButton("StyleCalssic", &e, 7);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Dark 4", &e, 8);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Dark 5", &e, 9);
                ImGui::TableNextColumn();
                ImGui::RadioButton("Dark 6", &e, 10);
                ImGui::TableNextColumn();
                switch (e)
                {
                case 1:
                    Theme::SetBlackGoldTheme();
                    break;
                case 2:
                    Theme::SetYesAnotherDarkTheme();
                    break;
                case 3:
                    Theme::SetDarkGrayTheme();
                    break;
                case 4:
                    Theme::SetCorporateGrayTheme();
                    break;
                case 5:
                    Theme::SetYetAnotherDarkTheme();
                    break;
                case 6:
                    Theme::SetSoftDarkRedTheme();
                    break;
                case 7:
                    Theme::SetClassicSteamHalfLifeTheme();
                    break;
                case 8:
                    Theme::SetAnotherDarkThemeReally();
                    break;
                case 9:
                    Theme::SetDarkGreenBlueTheme();
                    break;
                case 10:
                    Theme::SetDarkRedTheme();
                    break;
                }
                ImGui::Text("Hello from another window!");
                ImGui::Text("App Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                if (ImGui::Button("Close"))
                    show_another_window = false;
                ImGui::End();
            }
            ImGui::TextWrapped("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }
    }

    void DrawImGuiMenu()
    {
        if (init && glHeight != 0)
        {
            ImGuiIO &io = ImGui::GetIO();
            static bool WantTextInputLast = false;
            if (io.WantTextInput && !WantTextInputLast)
            {
                LOGI("DrawImGuiMenu: Displaying keyboard");
                displayKeyboard(true);
            }
            WantTextInputLast = io.WantTextInput;
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplAndroid_NewFrame(glWidth, glHeight);
            ImGui::NewFrame();
            DrawMenu();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            ImGui::EndFrame();
        }
    }

} // namespace Menu
