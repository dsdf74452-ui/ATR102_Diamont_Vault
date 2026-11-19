cmake_minimum_required(VERSION 3.22)

# Runtime de MSVC enlazado estaticamente: el plugin no exige instalar el VC++ Redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ATR102 VERSION 1.0.0 LANGUAGES C CXX)

option(ATR_COPY_AFTER_BUILD "Copiar el VST3 a la carpeta del sistema tras compilar (puede requerir admin)" OFF)
option(ATR_BUILD_STANDALONE "Compilar tambien la version Standalone para pruebas" OFF)
set(JUCE_SOURCE_DIR "" CACHE PATH "Ruta a una copia local de JUCE (opcional; si esta vacia se descarga)")

if(JUCE_SOURCE_DIR)
    add_subdirectory(${JUCE_SOURCE_DIR} JUCE EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(ATR_FORMATS VST3)
if(ATR_BUILD_STANDALONE)
    list(APPEND ATR_FORMATS Standalone)
endif()

juce_add_plugin(ATR102
    COMPANY_NAME                "Diamont Vault"
    COMPANY_COPYRIGHT           "(c) 2026 Diamont Vault(R). All rights reserved."
    PLUGIN_MANUFACTURER_CODE    Dvlt
    PLUGIN_CODE                 Atr1
    FORMATS                     ${ATR_FORMATS}
    PRODUCT_NAME                "ATR-102 MASTERING TAPE RECORDER PRO"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES             Fx Distortion
    COPY_PLUGIN_AFTER_BUILD     ${ATR_COPY_AFTER_BUILD})

juce_add_binary_data(ATR102Assets SOURCES
    assets/bg_off.png
    assets/bg_on.png
    assets/reel_left.png
    assets/reel_right.png
    assets/needle.png
    assets/strip_patch.png)

target_sources(ATR102 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/TapeEngine.cpp)

target_compile_definitions(ATR102 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(ATR102
    PRIVATE
        ATR102Assets
        juce::juce_audio_utils
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# Pruebas offline del motor (sin JUCE): cmake --build build --target test_engine
add_executable(test_engine EXCLUDE_FROM_ALL tools/test_engine.cpp Source/TapeEngine.cpp)
target_include_directories(test_engine PRIVATE Source)
