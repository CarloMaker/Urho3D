#
# Copyright (c) 2008-2013 the Urho3D project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

# Define target name
set (TARGET_NAME LuaScript)

# Optionally enable Lua / C++ wrapper safety checks
if (NOT ENABLE_SAFE_LUA)
    add_definitions (-DTOLUA_RELEASE)
endif ()

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

# Define generated source files
file (MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/generated)
file (GLOB API_PKG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkgs/*.pkg)
foreach (API_PKG_FILE ${API_PKG_FILES})
    get_filename_component (NAME ${API_PKG_FILE} NAME)
    string (REGEX REPLACE LuaAPI\\.pkg$ "" API ${NAME})
    set (GEN_CPP_FILE generated/${API}LuaAPI.cpp)
    set (GEN_CPP_FILES ${GEN_CPP_FILES} ${GEN_CPP_FILE})
    file (GLOB PKG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/pkgs/${API}/*.pkg)
    add_custom_command (OUTPUT ${GEN_CPP_FILE} 
        COMMAND ${PROJECT_ROOT_DIR}/Bin/tolua++ -L basic.lua -o ${CMAKE_CURRENT_BINARY_DIR}/${GEN_CPP_FILE} ${NAME} DEPENDS ${API_PKG_FILE} ${PKG_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/pkgs/basic.lua
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/pkgs
        COMMENT "Generating tolua++ API binding on the fly for ${API}")
endforeach ()
set (SOURCE_FILES ${SOURCE_FILES} ${GEN_CPP_FILES})

# Define dependency libs
set (LIBS ../../Engine/Audio ../../Engine/Container ../../Engine/Core ../../Engine/Engine ../../Engine/Graphics ../../Engine/Input ../../Engine/IO ../../Engine/Math ../../Engine/Navigation ../../Engine/Network
    ../../Engine/Physics ../../Engine/Resource ../../Engine/Scene ../../Engine/UI)
if (ENABLE_LUA_JIT)
    set (LIBS ${LIBS} ../../ThirdParty/LuaJIT/src)
else ()
    set (LIBS ${LIBS} ../../ThirdParty/Lua/src)
endif ()
set (LINK_LIBS_ONLY tolua++_static)
set (INCLUDE_DIRS_ONLY . ../../ThirdParty/Bullet/src ../../ThirdParty/kNet/include ../../ThirdParty/tolua++/include)

# Setup target
enable_pch ()
setup_library ()

# For IOS, Android, and Raspberry-Pi cross-compiling build to be successful, the "tolua++" tool must be already present in the "Bin" folder
# One way to achieve this is to first do a native desktop build where all the tools would be generated, then switch to IOS/Android/Raspberry-Pi cross-compiling build
# Intentionally do not include NOT RASPI in the condition below because the dependency is still required for Raspberry Pi native build
if (NOT IOS AND NOT ANDROID)
    # Add dependency to the "tolua++" bin tool
    add_dependencies (${TARGET_NAME} tolua++)
endif ()
