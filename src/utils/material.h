#pragma once

#include "utils/gl_types.h"
#include "utils/shader.h"

struct Material {

	template<typename T>
	void setUniform(std::string& name, T value);

	void updateUniforms(Shader& shader);
	void updateUniforms();

	std::vector<Texture> textures;
	std::vector<std::string> texture_paths;
	Shader* shader;

	std::unordered_map<std::string, int> uniformInts;
	std::unordered_map<std::string, float> uniformFloats;
	std::unordered_map<std::string, glm::vec3> uniformVec3s;
	std::unordered_map<std::string, glm::mat4> uniformMat4s;

};
