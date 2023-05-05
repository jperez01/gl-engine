#include "material.h"

template<typename T>
inline void Material::setUniform(std::string& name, T value)
{
	if constexpr (std::is_same<T, int>()) { uniformInts[name] = value; }
	else if constexpr (std::is_same<T, float>()) { uniformFloats[name] = value; }
	else if constexpr (std::is_same<T, glm::vec3>()) { uniformVec3s[name] = value; }
	else if constexpr (std::is_same<T, glm::mat4>()) { uniformMat4s[name] = value; }
	else {
		static_assert(false, "Unsupported data type in Material");
	}
}

void Material::updateUniforms(Shader& shader)
{
	for (auto& pair : uniformInts) {
		shader.setInt(pair.first, pair.second);
	}
	for (auto& pair : uniformFloats) {
		shader.setFloat(pair.first, pair.second);
	}

	for (auto& pair : uniformVec3s) {
		shader.setVec3(pair.first, pair.second);
	}
	for (auto& pair : uniformMat4s) {
		shader.setMat4(pair.first, pair.second);
	}
}

void Material::updateUniforms()
{
	if (shader != nullptr) {
		for (auto& pair : uniformInts) {
			shader->setInt(pair.first, pair.second);
		}
		for (auto& pair : uniformFloats) {
			shader->setFloat(pair.first, pair.second);
		}

		for (auto& pair : uniformVec3s) {
			shader->setVec3(pair.first, pair.second);
		}
		for (auto& pair : uniformMat4s) {
			shader->setMat4(pair.first, pair.second);
		}
	}
}
