
cmake_minimum_required(VERSION 3.4.1)

# Use C++17 as in Android.mk
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Mirror Application.mk / Android.mk flags
add_compile_options(
	-w
	-s
	-Wno-error=format-security
	-fvisibility=hidden
	-fpermissive
	-fexceptions
	-frtti
	-Wall
	-Wno-error=c++11-narrowing
	-Werror
)

# Sources (copied from Android.mk LOCAL_SRC_FILES)
set(SRC_FILES
	Main.cpp
	Menu/Menu.cpp
	Menu/Themes.cpp
	Includes/JNIStuff.cpp
	InputHook.cpp
	Substrate/hde64.c
	Substrate/SubstrateDebug.cpp
	Substrate/SubstrateHook.cpp
	Substrate/SubstratePosixMemory.cpp
	Substrate/SymbolFinder.cpp
	KittyMemory/KittyMemory.cpp
	KittyMemory/MemoryPatch.cpp
	KittyMemory/MemoryBackup.cpp
	KittyMemory/KittyUtils.cpp
	And64InlineHook/And64InlineHook.cpp
	ImGui/imgui.cpp
	ImGui/imgui_draw.cpp
	ImGui/imgui_demo.cpp
	ImGui/imgui_widgets.cpp
	ImGui/imgui_tables.cpp
	ImGui/backends/imgui_impl_opengl3.cpp
	ImGui/backends/imgui_impl_android.cpp
)

# Declares and builds the native shared library.
add_library(native-lib SHARED ${SRC_FILES})

# Import the prebuilt static library libdobby (path matches Android.mk's libraries/$(TARGET_ARCH_ABI)/libdobby.a)
add_library(libdobby STATIC IMPORTED)
set_target_properties(libdobby PROPERTIES
	IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libraries/${ANDROID_ABI}/libdobby.a"
)

# Find standard Android system libraries
find_library(log-lib log)

target_link_libraries(native-lib
	PRIVATE
		libdobby
		${log-lib}
		android
		EGL
		GLESv3
)


# Add all relevant include directories for headers
target_include_directories(native-lib PRIVATE
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/Includes
	${CMAKE_SOURCE_DIR}/Menu
	${CMAKE_SOURCE_DIR}/KittyMemory
	${CMAKE_SOURCE_DIR}/Substrate
	${CMAKE_SOURCE_DIR}/ImGui
	${CMAKE_SOURCE_DIR}/ImGui/backends
	${CMAKE_SOURCE_DIR}/And64InlineHook
	${CMAKE_SOURCE_DIR}/Data
)
