#pragma once
#include <unordered_map>
#include <string>
struct ShaderData {
	const unsigned char* data;
	size_t length;
};

extern std::unordered_map<std::string, ShaderData> g_shaders;
