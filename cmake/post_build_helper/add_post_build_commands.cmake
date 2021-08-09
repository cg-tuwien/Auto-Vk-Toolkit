function(add_post_build_commands targetName glslDirectory spvDirectory assetsDirectory assets symlinks)
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
    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -E make_directory ${spvDirectory}
        COMMENT "Post Build Helper: creating shader directory")
    add_custom_command(TARGET ${targetName} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -Dpbh_TargetName=hello_world
            -Dpbh_glslDirectory=${glslDirectory}
            -Dpbh_spvDirectory=${spvDirectory}
            -P compile_shaders_command.cmake
        COMMENT "Post Build Helper: compiling shaders"
        WORKING_DIRECTORY ${Gears_Vk_SOURCE_DIR}/cmake/post_build_helper)

    # assets
    if (assets)
        add_custom_command(TARGET ${targetName} POST_BUILD
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
                        add_custom_command(TARGET ${targetName} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E create_symlink
                                ${subAsset}
                                "${assetsDirectory}/${subAssetFileName}"
                            COMMENT "Post Build Helper: creating symlink for asset ${subAssetFileName}")
                    endforeach(subAsset)
                else()
                    add_custom_command(TARGET ${targetName} POST_BUILD
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
                        add_custom_command(TARGET ${targetName} POST_BUILD
                            COMMAND ${CMAKE_COMMAND} -E ${copyAssetCommand}
                                ${subAsset}
                                ${assetsDirectory}/${subAssetFileName}
                            COMMENT "Post Build Helper: copying asset ${subAssetFileName}")
                    endforeach(subAsset)
                else(IS_DIRECTORY "${asset}")
                    add_custom_command(TARGET ${targetName} POST_BUILD
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
        if (CMAKE_BUILD_TYPE STREQUAL "Release" OR gvk_ReleaseDLLsOnly)
            set(gvk_DLLDirectory "${Gears_Vk_SOURCE_DIR}/external/release/bin/x64")
        else()
            set(gvk_DLLDirectory "${Gears_Vk_SOURCE_DIR}/external/debug/bin/x64")
        endif (CMAKE_BUILD_TYPE STREQUAL "Release" OR gvk_ReleaseDLLsOnly)
        file(GLOB dllsToCopy "${gvk_DLLDirectory}/*.dll")
        foreach(dllToCopy ${dllsToCopy})
            get_filename_component(dllFileName ${dllToCopy} NAME)
            if(symlinks)
                add_custom_command(TARGET ${targetName} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E create_symlink
                        ${dllToCopy}
                        $<TARGET_FILE_DIR:${targetName}>/${dllFileName}
                    COMMENT "Post Build Helper: creating symlink for DLL ${dllFileName}")
            else()
                add_custom_command(TARGET ${targetName} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${dllToCopy}
                        $<TARGET_FILE_DIR:${targetName}>/${dllFileName}
                    COMMENT "Post Build Helper: copying DLL ${dllFileName}")
            endif(symlinks)
        endforeach(dllToCopy)
    else()
        message(STATUS "Shared libraries should be in the rpath of ${targetName} and are therefore not copied to the target location. Also, the location of built libraries should be configured via CMAKE_RUNTIME_OUTPUT_DIRECTORY, etc.")
    endif (WIN32)
endfunction()
