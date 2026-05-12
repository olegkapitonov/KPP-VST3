
# Back-compat: newer VST3 SDKs renamed SMTG_ADD_VSTGUI to
# SMTG_ENABLE_VSTGUI_SUPPORT. Each plugin's CMakeLists.txt still gates on the
# old name, so derive it when the new one is set.
if(SMTG_ENABLE_VSTGUI_SUPPORT AND NOT DEFINED SMTG_ADD_VSTGUI)
    set(SMTG_ADD_VSTGUI ON)
endif()

# Default IDE folder for these plugins, in case the host project hasn't set
# one. Newer SDKs no longer define SDK_IDE_MYPLUGINS_FOLDER themselves, and
# without it set_target_properties(... PROPERTIES ${SDK_IDE_MYPLUGINS_FOLDER})
# errors with "set_target_properties called with illegal arguments".
if(NOT DEFINED SDK_IDE_MYPLUGINS_FOLDER)
    set(SDK_IDE_MYPLUGINS_FOLDER FOLDER MyPlugins)
endif()

add_subdirectory(kpp_fuzz)
add_subdirectory(kpp_bluedream)
add_subdirectory(kpp_distruction)
add_subdirectory(kpp_deadgate)
add_subdirectory(kpp_single2humbucker)
add_subdirectory(kpp_octaver)
add_subdirectory(kpp_tubeamp)
