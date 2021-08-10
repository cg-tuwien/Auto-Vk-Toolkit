function(get_shaders shaders glslDirectory)
    # TODO: check if other file extensions should be included (e.g. .glsl)
    # TODO: check if directories should be checked
    file(GLOB_RECURSE glslSourceFiles
            "${glslDirectory}/*.vert"
            "${glslDirectory}/*.tesc"
            "${glslDirectory}/*.tese"
            "${glslDirectory}/*.geom"
            "${glslDirectory}/*.frag"
            "${glslDirectory}/*.comp"
            "${glslDirectory}/*.rgen"
            "${glslDirectory}/*.rint"
            "${glslDirectory}/*.rahit"
            "${glslDirectory}/*.rchit"
            "${glslDirectory}/*.rmiss"
            "${glslDirectory}/*.rcall"
            "${glslDirectory}/*.mesh") # TODO: whats the common file extension for mesh shaders?
    set(${shaders} ${glslSourceFiles} PARENT_SCOPE)
endfunction(get_shaders)

function(make_shader_target shaderTarget target glslDirectory spvDirectory)
    if(UNIX)
        set(glslangValidator "glslangValidator")
    else()
        set(glslangValidator "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    endif(UNIX)

    get_shaders(glslSourceFiles ${glslDirectory})

    foreach(glslShader ${glslSourceFiles})
        get_filename_component(glslShaderFileName ${glslShader} NAME)
        set(spvShaderFile "${spvDirectory}/${glslShaderFileName}.spv")
        add_custom_command(
                OUTPUT ${spvShaderFile}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${spvDirectory}
                COMMAND ${glslangValidator}
                    --target-env vulkan1.2 -V ${glslShader} -o ${spvShaderFile}
                DEPENDS ${glslShader}
                COMMENT "Compiling GLSL shader ${glslShaderFileName} to SPIR-V shader ${spvShaderFile}"
        )
        list(APPEND spvShaders ${spvShaderFile})
        message(STATUS "Added GLSL shader ${glslShaderFileName} to dependencies of target ${target}")
    endforeach(glslShader)
    add_custom_target(${shaderTarget}
            DEPENDS ${spvShaders})
    add_dependencies(${target}
            ${shaderTarget})
endfunction()

function(compile_shaders targetName glslDirectory spvDirectory)
    if(UNIX)
        set(glslangValidator "glslangValidator")
    else()
        set(glslangValidator "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    endif(UNIX)

    get_shaders(glslSourceFiles ${glslDirectory})

    foreach(glslShader ${glslSourceFiles})
        get_filename_component(glslShaderFileName ${glslShader} NAME)
        set(spvShaderFile "${spvDirectory}/${glslShaderFileName}.spv")
        execute_process(COMMAND ${glslangValidator}
                --target-env vulkan1.2 -V ${glslShader} -o ${spvShaderFile})
        message(STATUS "Compiled GLSL shader ${glslShaderFileName} to SPIR-V shader ${spvShaderFile}")
    endforeach(glslShader)
endfunction()