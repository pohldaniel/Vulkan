#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <shaderc/shaderc.h>

struct CompilationInfo {
	const char* fileName;
	shaderc_shader_kind kind;
	std::vector<char> source;
	shaderc_compile_options_t options;
};


struct VkShader {
	std::vector<char> read_file(const char* filename);
	void write_file(const char* filename, std::vector<uint32_t>& data);
	void preprocess_shader(CompilationInfo& info);
	void compile_file_to_assembly(CompilationInfo& info);
	std::vector<uint32_t> compile_file(CompilationInfo& info);
	std::vector<VkShaderEXT> make_shader_objects(VkInstance instance, VkDevice logicalDevice, const char* name, std::vector<uint32_t>& vertexCode, std::vector<uint32_t>& fragmentCode, bool buildAndCompile = true);
};