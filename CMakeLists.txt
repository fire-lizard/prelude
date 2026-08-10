cmake_minimum_required(VERSION 3.10)

project(Prelude)

option(SOUND_DRIVER_MINIAUDIO "Use MiniAudio sound driver" ON)
option(BACKPORT_18            "Backport 1.8 updates"       ON)

if(WIN32 OR APPLE)
add_subdirectory("external/glfw-3.4")
add_subdirectory("external/glew-2.3.1" EXCLUDE_FROM_ALL)
else()
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
endif()

find_package(OpenGL REQUIRED)
if(NOT WIN32 AND NOT APPLE)
find_package(X11 REQUIRED)
endif()

set(SOURCE_FILES
	Source/actionmenuclass.cpp
	Source/actions.cpp
	Source/area.cpp
	Source/attacks.cpp
	Source/aura.cpp
	Source/Barter.cpp
	Source/blood.cpp
	Source/castaura.cpp
	Source/cavewall.cpp
	Source/CharacterWin.cpp
	Source/Chunks.cpp
	Source/combatdemomain.cpp
	Source/CombatManager.cpp
	Source/containers.cpp
	Source/Corpse.cpp
	Source/CreatePartyWin.cpp
	Source/creatures.cpp
	Source/deathwin.cpp
	Source/EditRegion.cpp
	Source/entrance.cpp
	Source/equipeditorwin.cpp
	Source/equipobject.cpp
	Source/equipwin.cpp
	Source/events.cpp
	Source/explosion.cpp
	Source/fireball.cpp
	Source/Flags.cpp
	Source/forest.cpp
	Source/fountain.cpp
	Source/gameitem.cpp
	Source/Generatworld.cpp
	Source/healaura.cpp
	Source/inventorywin.cpp
	Source/items.cpp
	Source/journal.cpp
	Source/LoadObject.cpp
	Source/Locator.cpp
	Source/MainWindow.cpp
	Source/maplocator.cpp
	Source/mapwin.cpp
	Source/meshfx.cpp
	Source/minimap.cpp
	Source/missile.cpp
	Source/modifiers.cpp
	Source/MovePointer.cpp
	Source/Objects.cpp
	Source/Party.cpp
	Source/path.cpp
	Source/pattern.cpp
	Source/peopleedit.cpp
	Source/Pickpocket.cpp
	Source/portals.cpp
	Source/randomevent.cpp
	Source/regions.cpp
	Source/registration.cpp
	Source/script.cpp
	Source/scriptfuncs.cpp
	Source/signs.cpp
	Source/skillwin.cpp
	Source/spellbook.cpp
	Source/spellfuncs.cpp
	Source/Spells.cpp
	Source/StartScreen.cpp
	Source/texturemanager.cpp
	Source/things.cpp
	Source/translucentwindow.cpp
	Source/updateworld.cpp
	Source/walls.cpp
	Source/water.cpp
	Source/World.cpp
	Source/worldedit.cpp
	Source/zsactionwindow.cpp
	Source/ZSaskwin.cpp
	Source/zsbrushwindow.cpp
	Source/ZSbutton.cpp
	Source/ZSCaveEdit.cpp
	Source/zsconfirm.cpp
	Source/zscontainer.cpp
	Source/ZScutscene.cpp
	Source/ZSDescribe.cpp
	Source/ZSEditWindow.cpp
	Source/ZSEngine.cpp
	Source/zsfire.cpp
	Source/ZSFloatSpin.cpp
	Source/ZSFontEngine.cpp
	Source/zsgetnumber.cpp
	Source/zsgettarget.cpp
	Source/ZSGetText.cpp
	Source/ZSGraphics.cpp
	Source/ZSHelpWin.cpp
	Source/ZSinput.cpp
	Source/zsIntSpin.cpp
	Source/zsitemslot.cpp
	Source/zslistbox.cpp
	Source/ZSMainDescribe.cpp
	Source/zsmenubar.cpp
	Source/zsMessage.cpp
	Source/ZSModel.cpp
	Source/ZSModelEx.cpp
	Source/ZSobjectwindow.cpp
	Source/zsOldlistbox.cpp
	Source/ZSOptions.cpp
	Source/ZSparticle.cpp
	Source/ZSportrait.cpp
	Source/ZSProgress.cpp
	Source/ZSREGIONEDIT.cpp
	Source/ZSRest.cpp
	Source/ZSSaveLoad.cpp
	Source/zssaychar.cpp
	Source/ZSScriptWindow.cpp
	Source/ZSShrinkWindow.cpp
	Source/ZSsound.cpp
	Source/ZSSpellWindow.cpp
	Source/ZSTalk.cpp
	Source/ZSText.cpp
	Source/ZStextures.cpp
	Source/ZStoolwindow.cpp
	Source/ZSutilities.cpp
	Source/ZSVerticalScroll.cpp
	Source/zsweapontrace.cpp
	Source/ZSwindow.cpp
)

set(OPENGL_FILES
	Prelude/birdeye.cpp
	Prelude/birdeye.h
	Prelude/bmp.cpp
	Prelude/bmp.h
	Prelude/opengl1.cpp
	Prelude/opengl1.h
	Prelude/opengl_draw.cpp
	Prelude/opengl_input.cpp
	Prelude/opengl_math.cpp
)

if(WIN32)
set(OS_FILES
	Script1.rc
)
else()
set(OS_FILES
	Prelude/linux_files.cpp
	Prelude/linux_graphics.cpp
	Prelude/linux_threading.cpp
	Prelude/linux_utils.cpp
)
endif()

if(SOUND_DRIVER_MINIAUDIO)
set(SOUND_FILES
	Prelude/miniaudio.c
	Prelude/stb_vorbis.c
)
else()
set(SOUND_FILES
)
endif()

add_executable(Prelude
	${SOURCE_FILES}
	${OPENGL_FILES}
	${OS_FILES}
	${SOUND_FILES}
)

if ((CMAKE_CXX_COMPILER_ID MATCHES "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
target_compile_options(Prelude PRIVATE
	-Wall $<$<COMPILE_LANGUAGE:CXX>:-Wno-conversion-null>
)
endif()

# The game makes a lot of use of variants held by void* and it's a 32bit game so casts from int to void* are benign
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
target_compile_options(Prelude PRIVATE
	$<$<COMPILE_LANGUAGE:CXX>:-Wno-int-to-void-pointer-cast>
	-Wno-unused-variable -Wno-unused-but-set-variable
)
endif()
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
target_compile_options(Prelude PRIVATE
	-Wno-int-to-pointer-cast
	$<$<COMPILE_LANGUAGE:CXX>:-Wno-class-memaccess>
	-Wno-unused-variable
	-Wno-unused-but-set-variable
)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(Prelude PRIVATE -fsanitize=address)
target_link_options(Prelude PRIVATE -fsanitize=address)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
target_compile_options(Prelude PRIVATE -D_CRT_SECURE_NO_WARNINGS)
endif()

target_compile_definitions(Prelude PRIVATE OPENGL)

if(SOUND_DRIVER_MINIAUDIO)
	target_compile_definitions(Prelude PRIVATE PTD_SOUND_DRIVER_MINIAUDIO)
endif()
if(BACKPORT_18)
	target_compile_definitions(Prelude PRIVATE BACKPORT_18)
endif()

if(WIN32 OR APPLE)
target_include_directories(Prelude PUBLIC
	"${PROJECT_SOURCE_DIR}/Prelude"
	"${PROJECT_SOURCE_DIR}/Prelude/DXSource"
	"${PROJECT_SOURCE_DIR}/external/glew-2.3.1/include"
)
else()
target_include_directories(Prelude PUBLIC
	"${PROJECT_SOURCE_DIR}/Prelude"
	"${PROJECT_SOURCE_DIR}/Prelude/DXSource"
)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
set_target_properties(Prelude PROPERTIES WIN32_EXECUTABLE TRUE)
target_link_libraries(Prelude PUBLIC
	OpenGL::GL glfw libglew_static Winmm
)
elseif(APPLE)
target_compile_definitions(Prelude PRIVATE GL_SILENCE_DEPRECATION)
target_link_libraries(Prelude PUBLIC
	OpenGL::GL glfw libglew_static
	"-framework CoreFoundation" "-framework CoreAudio" "-framework AudioToolbox"
)
else()
target_link_libraries(Prelude PUBLIC
	OpenGL GLEW glfw X11
)
endif()
