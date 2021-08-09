function(compile_shaders targetName glslDirectory spvDirectory)
    if(UNIX)
        set(glslangValidator "glslangValidator")
    else()
        set(glslangValidator "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
    endif(UNIX)

    # TODO: check if other file extensions should be included (e.g. .glsl)
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

    foreach(glslShader ${glslSourceFiles})
        get_filename_component(glslShaderFileName ${glslShader} NAME)
        set(spvShaderFile "${spvDirectory}/${glslShaderFileName}.spv")
        execute_process(COMMAND ${glslangValidator}
                --target-env vulkan1.2 -V ${glslShader} -o ${spvShaderFile})
        message(STATUS "Compiled GLSL shader ${glslShaderFileName} to SPIR-V shader ${spvShaderFile}")
    endforeach(glslShader)
endfunction()