#include <iostream>
#include <fstream>
#include <sstream>
#include "vk_shader.h"

void VkShader::preprocess_shader(CompilationInfo& info) {	
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compilation_result_t result = shaderc_compile_into_preprocessed_text(compiler, info.source.data(), info.source.size(), info.kind, info.fileName, "main", info.options);

	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
		std::cout << "Error: preprocess" << std::endl;
	}

	//copy result into info for next compilation operation
	const char* src = reinterpret_cast<const char*>(shaderc_result_get_bytes(result));
	size_t newSize = reinterpret_cast<const char*>(shaderc_result_get_bytes(result)) + shaderc_result_get_length(result) / sizeof(char) - src;
	info.source.resize(newSize);
	memcpy(info.source.data(), src, newSize);

	//Log output for fun
	//std::cout << "---- Preprocessed GLSL source code ----" << std::endl;
	//std::string output = { info.source.data(), info.source.data() + info.source.size() };
	//std::cout << output << std::endl;
}

void VkShader::compile_file_to_assembly(CompilationInfo& info) {
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compilation_result_t result = shaderc_compile_into_spv_assembly(compiler, info.source.data(), info.source.size(), info.kind, info.fileName, "main", info.options);

	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
		std::cout << "Error: toSpvAssembly" << std::endl;
	}

	//copy result into info for next compilation operation
	const char* src = reinterpret_cast<const char*>(shaderc_result_get_bytes(result));
	size_t newSize = reinterpret_cast<const char*>(shaderc_result_get_bytes(result)) + shaderc_result_get_length(result) / sizeof(char) - src;
	info.source.resize(newSize);
	memcpy(info.source.data(), src, newSize);

	//Log output for fun
	//std::cout << "---- SPIR-V Assembly code ----" << std::endl;
	//std::string output = { info.source.data(), info.source.data() + info.source.size() };
	//std::cout << output << std::endl;
}

std::vector<uint32_t> VkShader::compile_file(CompilationInfo& info) {
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compilation_result_t result = shaderc_assemble_into_spv(compiler, info.source.data(), info.source.size(), info.options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status::shaderc_compilation_status_success) {
		std::cout << "Error: compile" << std::endl;
	}

	//copy result to the final output
	const uint32_t* src = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));
	size_t wordCount = shaderc_result_get_length(result) /sizeof(uint32_t);
	std::vector<uint32_t> output(wordCount);
	memcpy(output.data(), src, shaderc_result_get_length(result));

	//Log output for fun
	//std::cout << "---- SPIR-V Binary Code ----" << std::endl;
	//std::cout << "Magic Number: " << std::endl;
	//std::stringstream converter;
	//converter << output[0];
	//std::cout << converter.str() << std::endl;

	return output;
}

std::vector<VkShaderEXT> VkShader::make_shader_objects(VkInstance instance, VkDevice logicalDevice, const char* name, std::vector<uint32_t>& vertexCode, std::vector<uint32_t>& fragmentCode) {
		CompilationInfo info;
		info.options = shaderc_compile_options_initialize();

		shaderc_compile_options_set_target_env(info.options, shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
		shaderc_compile_options_set_target_env(info.options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
		shaderc_compile_options_set_source_language(info.options, shaderc_source_language_glsl);
		shaderc_compile_options_set_target_spirv(info.options, shaderc_spirv_version_1_3);
		shaderc_compile_options_set_optimization_level(info.options, shaderc_optimization_level_zero);
		//shaderc_compile_options_set_suppress_warnings(info.options);
		//shaderc_compile_options_set_warnings_as_errors(info.options);
		//shaderc_compile_options_set_auto_bind_uniforms(info.options, false);
		//shaderc_compile_options_set_preserve_bindings(info.options, false);
		//shaderc_compile_options_set_vulkan_rules_relaxed(info.options, false);
		//shaderc_compile_options_set_auto_combined_image_sampler(info.options, false);
		std::stringstream filenameBuilder;
		std::string filename;

		VkShaderCreateFlagsEXT flags = VkShaderCreateFlagBitsEXT::VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
		VkShaderStageFlags nextStage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;

		filenameBuilder << "res/shader/" << name << ".vert";
		filename = filenameBuilder.str();
		filenameBuilder.str("");	
		info.fileName = filename.c_str();
		info.kind = shaderc_vertex_shader;
		info.source = read_file(info.fileName);
		preprocess_shader(info);
		compile_file_to_assembly(info);
		vertexCode = compile_file(info);

		filenameBuilder << "res/shader/" << name << ".frag";
		filename = filenameBuilder.str();
		filenameBuilder.str("");
		info.fileName = filename.c_str();
		info.kind = shaderc_fragment_shader;
		info.source = read_file(info.fileName);
		preprocess_shader(info);
		compile_file_to_assembly(info);

		

		fragmentCode = compile_file(info);
		//write_file("res/shader/shader3.frag.spv", fragmentCode);

		VkShaderCodeTypeEXT codeType = VkShaderCodeTypeEXT::VK_SHADER_CODE_TYPE_SPIRV_EXT;
		const char* pName = "main";

		VkShaderCreateInfoEXT vertexInfo = {};
		vertexInfo.flags = flags;
		vertexInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
		vertexInfo.nextStage = nextStage;
		vertexInfo.codeType = codeType;
		vertexInfo.codeSize = sizeof(uint32_t) * vertexCode.size();
		vertexInfo.pCode = vertexCode.data();
		vertexInfo.pName = pName;

		

		VkShaderCreateInfoEXT fragmentInfo = {};
		fragmentInfo.flags = flags;
		fragmentInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentInfo.codeType = codeType;
		fragmentInfo.codeSize = sizeof(uint32_t) * fragmentCode.size();
		fragmentInfo.pCode = fragmentCode.data();
		fragmentInfo.pName = pName;
		
		
		std::vector<VkShaderCreateInfoEXT> shaderInfo;
		shaderInfo.push_back(vertexInfo);
		shaderInfo.push_back(fragmentInfo);

		std::vector <VkShaderEXT> shaderEXTs;
		shaderEXTs.resize(2);

		/*auto vkCreateShadersEXT = (PFN_vkCreateShadersEXT)vkGetInstanceProcAddr(instance, "vkCreateShadersEXT");

		std::cout << "Pointer: " << vkCreateShadersEXT << std::endl;

		VkResult result = vkCreateShadersEXT(logicalDevice, 2, shaderInfo.data(), nullptr, shaderEXTs.data());

		if (result == VkResult::VK_SUCCESS) {
			std::cout << "Successfully made shaders" << std::endl;
			VkShaderEXT vertexShader = shaderEXTs[0];
			//deviceDeletionQueue.push_back([vertexShader](VkDevice device) {
			//	vkDestroyShaderEXT(device, vertexShader, nullptr);
			//});
			VkShaderEXT fragmentShader = shaderEXTs[1];
			//deviceDeletionQueue.push_back([fragmentShader](VkDevice device) {
			//	vkDestroyShaderEXT(device, fragmentShader, nullptr);
			//});
		}else {
			std::cout << "Shader creation failed" << std::endl;
		}*/
		return shaderEXTs;
}

std::vector<char> VkShader::read_file(const char* filename) {

	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		std::stringstream line_builder;
		line_builder << "Failed to load \""
			<< filename << "\"" << std::endl;
		std::string line = line_builder.str();
	}

	size_t filesize{ static_cast<size_t>(file.tellg()) };

	std::vector<char> buffer(filesize);
	file.seekg(0);
	file.read(buffer.data(), filesize);

	file.close();
	return buffer;
}

void VkShader::write_file(const char* filename, std::vector<uint32_t>& data) {
	std::ofstream fs(filename, std::ios::out | std::ios::binary | std::ios::app);
	fs.write(reinterpret_cast<char*>(&data), data.size() * sizeof(uint32_t));
	fs.close();
}