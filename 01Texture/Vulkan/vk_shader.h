#pragma once

#define VULKAN_HPP_NO_EXCEPTIONS
//
#include <deque>
#include <functional>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.h>
//#include <shaderc/shaderc.hpp>

struct CompilationInfo {
	const char* fileName;

	shaderc_shader_kind kind;
	std::vector<char> source;
	shaderc_compile_options_t options;
	//shaderc::CompileOptions options;
};



struct VkShader {
	std::vector<char> read_file(const char* filename);

	void preprocess_shader(CompilationInfo& info);
	void compile_file_to_assembly(CompilationInfo& info);
	std::vector<uint32_t> compile_file(CompilationInfo& info);
	std::vector<VkShaderEXT> make_shader_objects(VkInstance instance, VkDevice logicalDevice, const char* name, std::vector<uint32_t>& vertexCode, std::vector<uint32_t>& fragmentCode);
};