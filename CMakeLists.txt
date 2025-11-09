cmake_minimum_required(VERSION 3.15)

project(HairlessMidiSerial VERSION 0.5.0)

# Find JUCE
# Assume JUCE is installed or available via CMake
# You can either:
# 1. Set JUCE_DIR to your JUCE installation
# 2. Use FetchContent to download JUCE
# 3. Add JUCE as a submodule

# Option 1: If JUCE is installed locally, uncomment and set path:
# list(APPEND CMAKE_PREFIX_PATH "C:/JUCE")

# Option 2: Using FetchContent to download JUCE (recommended)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

# Add the plugin/app
juce_add_gui_app(HairlessMidiSerial
    PRODUCT_NAME "Hairless MIDI Serial Bridge"
    COMPANY_NAME "Projectgus"
    BUNDLE_ID "com.projectgus.hairlessmidiserial"
    ICON_BIG ""
    MICROPHONE_PERMISSION_ENABLED FALSE
    CAMERA_PERMISSION_ENABLED FALSE
    BLUETOOTH_PERMISSION_ENABLED FALSE
)

# Add source files
target_sources(HairlessMidiSerial PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/MidiSerialBridge.h
    Source/MidiSerialBridge.cpp
    Source/SerialPortManager.h
    Source/SerialPortManager.cpp
    Source/ModernLookAndFeel.h
    Source/ModernLookAndFeel.cpp
)

# Link JUCE modules
target_link_libraries(HairlessMidiSerial
    PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_basics
        juce::juce_audio_devices
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(HairlessMidiSerial PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:HairlessMidiSerial,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:HairlessMidiSerial,JUCE_VERSION>"
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(HairlessMidiSerial PRIVATE setupapi)
endif()
