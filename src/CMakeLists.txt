add_subdirectory(kcm)

set(better_blur_dx_woapi_SOURCES
    better_blur_dx_api.hpp
    blur.cpp
    blur_wobbly_api.qrc
    blur_cache.cpp
    main.cpp
    refraction_pass.cpp
    rounded_corners_pass.cpp
    utils.cpp
    window.cpp
    window_manager.cpp
)

kconfig_add_kcfg_files(better_blur_dx_woapi_SOURCES
    blurconfig.kcfgc
)

if(BBDX_X11)
    add_library(better_blur_dx_woapi MODULE ${better_blur_dx_woapi_SOURCES})
    target_link_libraries(better_blur_dx_woapi PRIVATE
        KDecoration3::KDecoration
        KF6::ConfigGui
        KWinX11::kwin
        ${XCB_LIBRARIES}
    )
    target_compile_definitions(better_blur_dx_woapi PRIVATE
                               KWIN_VERSION_MAJOR=${KWin_VERSION_MAJOR}
                               KWIN_VERSION_MINOR=${KWin_VERSION_MINOR}
                               KWIN_VERSION_PATCH=${KWin_VERSION_PATCH}
                               BBDX_X11)
    install(TARGETS better_blur_dx_woapi DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin-x11/effects/plugins)
else()
    add_library(better_blur_dx_woapi MODULE ${better_blur_dx_woapi_SOURCES})
    target_link_libraries(better_blur_dx_woapi PRIVATE
        KDecoration3::KDecoration
        KF6::ConfigGui
        KWin::kwin
        ${XCB_LIBRARIES}
    )
    target_compile_definitions(better_blur_dx_woapi PRIVATE
                               KWIN_VERSION_MAJOR=${KWin_VERSION_MAJOR}
                               KWIN_VERSION_MINOR=${KWin_VERSION_MINOR}
                               KWIN_VERSION_PATCH=${KWin_VERSION_PATCH})
    install(TARGETS better_blur_dx_woapi DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins)
endif()

if (BBDX_DEBUG)
    target_compile_definitions(better_blur_dx_woapi PRIVATE BBDX_DEBUG)
endif()

target_compile_options(better_blur_dx_woapi PRIVATE -Wall -Wextra -Wunused -Wpedantic)

# Header-only, QVariant-based inter-effect API. It deliberately exposes no C++
# plugin object ABI; consumers communicate through versioned EffectWindow data.
install(FILES better_blur_dx_api.hpp
        DESTINATION ${KDE_INSTALL_INCLUDEDIR}/better-blur-dx-wobbly-api)
