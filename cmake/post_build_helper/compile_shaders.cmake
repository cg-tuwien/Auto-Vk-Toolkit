function(get_shaders shaders glslDirectory)
    # TODO: check if other file extensions should be included (e.g. .glsl)
    file(GLOB_RECURSE glslSourceFiles RELATIVE ${glslDirectory}
        "*.vert"
        "*.tesc"
        "*.tese"
        "*.geom"
        "*.frag"
        "*.comp"
        "*.rgen"
        "*.rint"
        "*.rahit"
        "*.rchit"
        "*.rmiss"
        "*.rcall"
        "*.mesh")  # TODO: whats the common file extension for mesh shaders?
    set(${shaders} ${glslSourceFiles} PARENT_SCOPE)
endfunction(get_shaders)

function(make_shader_target shaderTarget target glslDirectory spvDirectory)
    find_program(glslangValidator glslangValidator
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/x86_64/bin"
        DOC "The glslangValidator executable.")

    get_shaders(glslSourceFiles ${glslDirectory})

    foreach(glslShader ${glslSourceFiles})
        set(spvShaderFile "${spvDirectory}/${glslShader}.spv")
        set(glslShaderAbsolute "${glslDirectory}/${glslShader}")
        get_filename_component(glslShaderFileName ${glslShader} NAME)
        get_filename_component(spvShaderDirectory ${spvShaderFile} DIRECTORY)
        add_custom_command(
            OUTPUT ${spvShaderFile}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${spvShaderDirectory}
            COMMAND ${glslangValidator}
                --target-env vulkan1.2 -V ${glslShaderAbsolute} -o ${spvShaderFile}
            DEPENDS ${glslShaderAbsolute}
            COMMENT "Compiling GLSL shader ${glslShaderFileName} to SPIR-V shader ${spvShaderFile}")
        list(APPEND spvShaders ${spvShaderFile})
        message(STATUS "Added GLSL shader ${glslShaderFileName} to dependencies of target ${target}")
    endforeach(glslShader)
    add_custom_target(${shaderTarget}
            DEPENDS ${spvShaders})
    add_dependencies(${target}
            ${shaderTarget})
endfunction()

function(compile_shaders targetName glslDirectory spvDirectory)
    find_program(glslangValidator glslangValidator
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/x86_64/bin"
        DOC "The glslangValidator executable.")

    get_shaders(glslSourceFiles ${glslDirectory})

    foreach(glslShader ${glslSourceFiles})
        set(spvShaderFile "${spvDirectory}/${glslShader}.spv")
        get_filename_component(glslShaderFileName ${glslShader} NAME)
        get_filename_component(spvShaderDirectory ${spvShaderFile} DIRECTORY)
        execute_process(COMMAND ${glslangValidator}
            --target-env vulkan1.2 -V "${glslDirectory}/${glslShader}" -o ${spvShaderFile})
        message(STATUS "Compiled GLSL shader ${glslShaderFileName} to SPIR-V shader ${spvShaderFile}")
    endforeach(glslShader)
endfunction()