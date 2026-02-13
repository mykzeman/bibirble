cmake_minimum_required(VERSION 3.16)
project(Bibirble)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try common locations for wxWidgets on Windows before calling find_package.
# If you have WXWIN set in your environment or wxWidgets is installed to C:/wxWidgets,
# this helps CMake locate it.
if(NOT DEFINED wxWidgets_ROOT_DIR)
    if(DEFINED ENV{WXWIN})
        set(wxWidgets_ROOT_DIR $ENV{WXWIN})
    elseif(EXISTS "C:/wxWidgets")
        set(wxWidgets_ROOT_DIR "C:/wxWidgets")
    endif()
endif()

# Find wxWidgets (explicit COMPONENTS list)
find_package(wxWidgets COMPONENTS core base REQUIRED)
include(${wxWidgets_USE_FILE})

# Find nlohmann_json (for JSON parsing)
find_package(nlohmann_json REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/BibirbleWindow.cpp
    src/BibirbleWindow.h
    src/GameRow.cpp
    src/GameRow.h
    src/BibleData.cpp
    src/BibleData.h
    src/loading_dialog.h
    src/menu_wireframe.h
    src/wx_callafter_compat.h
)

# Create executable
add_executable(Bibirble ${SOURCES})

# Link libraries
target_link_libraries(Bibirble ${wxWidgets_LIBRARIES} nlohmann_json::nlohmann_json)

# Set startup project on Windows
if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Bibirble)
endif()
