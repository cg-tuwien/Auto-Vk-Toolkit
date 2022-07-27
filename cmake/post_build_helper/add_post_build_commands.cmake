include(${Auto_Vk_Toolkit_SOURCE_DIR}/cmake/post_build_helper/compile_shaders.cmake)

function(add_post_build_commands target glslDirectory spvDirectory assetsDirectory assets symlinks)
    # add precompiled headers from Auto-Vk-Toolkit
    target_precompile_headers(${target} REUSE_FROM Auto_Vk_Toolkit)

    # test if symbolic links are supported
    if (symlinks)
        message(STATUS "Symbolic links requested for dependencies. Checking for support...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
                CMakeLists.txt
                "${CMAKE_CURRENT_BINARY_DIR}/SymlinkTest_YouCanRemoveThis"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE symlinksSupported)
        if (symlinksSupported)
            message(STATUS "Symbolic links not supported by current user, copying dependencies instead. Make sure the user running CMake has the required priviledges for creating symbolic links.")
            set(symlinks OFF)
        endif (symlinksSupported)
    endif (symlinks)

    # shaders
    make_shader_target("${target}_shaders"
        ${target}
        ${glslDirectory}
        ${spvDirectory})

    # assets
    if (assets)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND}
                -E make_directory ${assetsDirectory}
                COMMENT "Post Build Helper: creating assets directory")
        foreach(asset ${assets})
            get_filename_component(assetFileName ${asset} NAME)
            if(symlinks)
                if(IS_DIRECTORY "${asset}")
                    file(GLOB subAssets "${asset}/*")
                    foreach(subAsset ${subAssets})
                        get_filename_component(subAssetFileName ${subAsset} NAME)
                        add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E create_symlink
                                ${subAsset}
                                "${assetsDirectory}/${subAssetFileName}"
                            COMMENT "Post Build Helper: creating symlink for asset ${subAssetFileName}")
                    endforeach(subAsset)
                else()
                    add_custom_command(TARGET ${target} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E create_symlink
                            ${asset}
                            "${assetsDirectory}/${assetFileName}"
                        COMMENT "Post Build Helper: creating symlink for asset ${assetFileName}")
                endif()
            else()
                if(IS_DIRECTORY "${asset}")
                    file(GLOB subAssets "${asset}/*")
                    foreach(subAsset ${subAssets})
                        if (IS_DIRECTORY "${subAsset}")
                            set(copyAssetCommand copy_directory)
                        else (IS_DIRECTORY "${subAsset}")
                            set(copyAssetCommand copy_if_different)
                        endif (IS_DIRECTORY "${subAsset}")
                        get_filename_component(subAssetFileName ${subAsset} NAME)
                        add_custom_command(TARGET ${target} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E ${copyAssetCommand}
                                ${subAsset}
                                ${assetsDirectory}/${subAssetFileName}
                            COMMENT "Post Build Helper: copying asset ${subAssetFileName}")
                    endforeach(subAsset)
                else(IS_DIRECTORY "${asset}")
                    add_custom_command(TARGET ${target} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            ${asset}
                            ${assetsDirectory}/${assetFileName}
                        COMMENT "Post Build Helper: copying asset ${assetFileName}")
                endif(IS_DIRECTORY "${asset}")
            endif(symlinks)
        endforeach()
    endif(assets)

    # shared libraries
    if (WIN32)
        if (CMAKE_BUILD_TYPE STREQUAL "Release" OR avk_toolkit_ReleaseDLLsOnly)
            set(avk_toolkit_DLLDirectory "${Auto_Vk_Toolkit_SOURCE_DIR}/external/release/bin/x64")
        else()
            set(avk_toolkit_DLLDirectory "${Auto_Vk_Toolkit_SOURCE_DIR}/external/debug/bin/x64")
        endif (CMAKE_BUILD_TYPE STREQUAL "Release" OR avk_toolkit_ReleaseDLLsOnly)
        file(GLOB dllsToCopy "${avk_toolkit_DLLDirectory}/*.dll")
        foreach(dllToCopy ${dllsToCopy})
            get_filename_component(dllFileName ${dllToCopy} NAME)
            if(symlinks)
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                        ${dllToCopy}
                        $<TARGET_FILE_DIR:${target}>/${dllFileName}
                    COMMENT "Post Build Helper: creating symlink for DLL ${dllFileName}")
            else()
                add_custom_command(TARGET ${target} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${dllToCopy}
                        $<TARGET_FILE_DIR:${target}>/${dllFileName}
                    COMMENT "Post Build Helper: copying DLL ${dllFileName}")
            endif(symlinks)
        endforeach(dllToCopy)
    else()
        message(STATUS "Shared libraries should be in the rpath of ${target} and are therefore not copied to the target location. Also, the location of built libraries should be configured via CMAKE_RUNTIME_OUTPUT_DIRECTORY, etc.")
    endif (WIN32)
endfunction()
