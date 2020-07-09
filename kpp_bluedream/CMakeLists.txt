
if(SMTG_ADD_VSTGUI)
    set(plug_sources
        include/plugcontroller.h
        include/plugids.h
        include/plugprocessor.h
        include/version.h
        include/kpp_bluedream_dsp.h
        source/plugfactory.cpp
        source/plugcontroller.cpp
        source/plugprocessor.cpp
    )

    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/include/kpp_bluedream_dsp.h"
                       COMMAND faust "${CMAKE_CURRENT_SOURCE_DIR}/include/kpp_bluedream.dsp" -cn BluedreamDsp -o "${CMAKE_CURRENT_BINARY_DIR}/kpp_bluedream_dsp.h"
                       WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include"
                       COMMENT "Compiling FAUST code..."
                       )

    include_directories(${CMAKE_CURRENT_BINARY_DIR})

    #--- HERE change the target Name for your plug-in (for ex. set(target myDelay))-------
    set(target kpp_bluedream)

    smtg_add_vst3plugin(${target} ${plug_sources})
    set_target_properties(${target} PROPERTIES ${SDK_IDE_MYPLUGINS_FOLDER})
    target_include_directories(${target} PUBLIC ${VSTGUI_ROOT}/vstgui4)
    target_link_libraries(${target} PRIVATE base sdk vstgui_support)

    smtg_add_vst3_resource(${target} "resource/plug.uidesc")
    smtg_add_vst3_resource(${target} "resource/base_scale.png")
    smtg_add_vst3_resource(${target} "resource/light.png")
    smtg_add_vst3_resource(${target} "resource/onoff_button.png")

    if(SMTG_MAC)
        smtg_set_bundle(${target} INFOPLIST "${CMAKE_CURRENT_LIST_DIR}/resource/Info.plist" PREPROCESS)
    elseif(SMTG_WIN)
        target_sources(${target} PRIVATE resource/plug.rc)
    endif()
endif(SMTG_ADD_VSTGUI)
