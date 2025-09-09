#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_android.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "Themes.h"
#include "../Data/Fonts/Roboto-Regular.h"
#include "Includes/JNIStuff.h"

using namespace ImGui;
extern bool init;
extern int glWidth, glHeight;
void SetupImGui();

namespace Menu
{
   extern ImVec4 color;
   extern bool *p_open;
   void DrawMenu();
   void DrawImGuiMenu();
}