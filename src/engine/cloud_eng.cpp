#include "cloud_eng.h"
#include "utils/gl_funcs.h"
#include "ui/ui.h"

#include <random>

void CloudEngine::init_resources()
{
	worleyNoiseShader = Shader("clouds/volume.vert", "clouds/volume.frag");
	worleyNoiseCompute = ComputeShader("clouds/worleyNoise.comp");
	cubeBuffer = glutil::createUnitCube();

	worleyNoiseTexture = glutil::createTexture3D(worleyNoiseDimensions.x, worleyNoiseDimensions.y,
		worleyNoiseDimensions.z);
	glBindImageTexture(0, worleyNoiseTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);

	screenQuad.init();

	createWorleyNoiseInfo();
	createWorleyNoiseTexture();
}

void CloudEngine::render(std::vector<Model>& objs)
{
	glClearColor(1.0, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	glm::vec4 boundsMin = boxModelMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec4 boundsMax = boxModelMatrix * glm::vec4(1.0f);

	worleyNoiseShader.use();
	worleyNoiseShader.setMat4("projection", camera->getProjectionMatrix());
	worleyNoiseShader.setMat4("view", camera->getViewMatrix());
	worleyNoiseShader.setMat4("model", boxModelMatrix);
	worleyNoiseShader.setMat4("inverseModel", glm::inverse(boxModelMatrix));

	worleyNoiseShader.setVec3("cameraPos", camera->Position);
	worleyNoiseShader.setVec3("boundsMin", glm::vec3(boundsMin));
	worleyNoiseShader.setVec3("boundsMax", glm::vec3(boundsMax));

	worleyNoiseShader.setInt("numSteps", numSteps);
	worleyNoiseShader.setVec3("cloudColor", cloudColor);
	worleyNoiseShader.setFloat("cloudScale", cloudScale);
	worleyNoiseShader.setFloat("cloudOffset", cloudOffset);
	worleyNoiseShader.setFloat("densityMultiplier", densityMultiplier);
	worleyNoiseShader.setFloat("densityThreshold", densityThreshold);

	worleyNoiseShader.setInt("numStepsLight", numStepsLight);
	worleyNoiseShader.setVec3("lightPos", lightPos);
	worleyNoiseShader.setFloat("lightAbsorptionTowardsSun", lightAbsorptionTowardsSun);
	worleyNoiseShader.setFloat("lightAbsorptionThroughCloud", lightAbsorptionThroughCloud);
	worleyNoiseShader.setFloat("darknessThreshold", darknessThreshold);
	worleyNoiseShader.setVec3("backgroundColor", glm::vec3(1.0f, 0.0f, 0.0));
	worleyNoiseShader.setVec4("phaseParams", phaseParams);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, worleyNoiseTexture);
	worleyNoiseShader.setFloat("layer", layer);
	worleyNoiseShader.setInt("worleyNoiseTexture", 0);

	glBindVertexArray(cubeBuffer.VAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
}

void CloudEngine::handleImGui()
{
	ImGui::SliderFloat("cloud Scale", &cloudScale, 0.1f, 10.0f);
	ImGui::SliderFloat("cloud Offset", &cloudOffset, 0.0f, 1.0f);
	ImGui::SliderFloat("density Multiplier", &densityMultiplier, 1.0f, 10.0f);
	ImGui::SliderFloat("Density Threshold", &densityThreshold, 0.0f, 1.0f);

	ImGui::SliderFloat3("Cloud Color", (float*) &cloudColor, 0.0f, 1.0f);
	ImGui::SliderInt("Num Steps", &numSteps, 5, 100);

	ImGui::SliderFloat3("Light Position", (float*)&lightPos, -20.0f, 20.0f);
	ImGui::SliderInt("Num Steps for Light", &numStepsLight, 1, 100);
	ImGui::SliderFloat("lightAbsorptionTowardsSun", &lightAbsorptionTowardsSun, 0.0f, 1.0f);
	ImGui::SliderFloat("lightAbsorptionThroughCloud", &lightAbsorptionThroughCloud, 0.0f, 1.0f);
	ImGui::SliderFloat("darknessThreshold", &darknessThreshold, 0.0f, 1.0f);
	ImGui::SliderFloat4("Phase Params", (float*)&phaseParams, 0.0f, 1.0f);


	UI::manipulateMatrix(boxModelMatrix, *camera);
}

void CloudEngine::createWorleyNoiseInfo()
{
	int worleyPointsSize = cellsPerAxis * cellsPerAxis * cellsPerAxis;
	std::random_device rd;
	std::mt19937 mt(rd());
	std::uniform_real_distribution<float> offset(0.0f, 1.0f);
	std::vector<glm::vec3> points(worleyPointsSize * 3);
	
	float gridCellSize = 1.0f / cellsPerAxis;
	for (int x = 0; x < cellsPerAxis; x++) {
		for (int y = 0; y < cellsPerAxis; y++) {
			for (int z = 0; z < cellsPerAxis; z++) {
				for (int num = 0; num < 3; num++) {
					glm::vec3 randomOffset(offset(mt), offset(mt), offset(mt));
					glm::vec3 finalPosition = (glm::vec3(x, y, z) + randomOffset) * gridCellSize;

					int index = x + cellsPerAxis * (y + z * cellsPerAxis);

					points[index + worleyPointsSize * num] = finalPosition;
				}
			}
		}
	}

	glCreateBuffers(1, &worleyNoiseSSBO);
	glNamedBufferData(worleyNoiseSSBO, sizeof(glm::vec3) * points.size(), points.data(), GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, worleyNoiseSSBO);
}

void CloudEngine::createWorleyNoiseTexture()
{
	int worleyPointsSize = cellsPerAxis * cellsPerAxis * cellsPerAxis;

	worleyNoiseCompute.use();
	worleyNoiseCompute.setVec3("worleyTextureSize", worleyNoiseDimensions);
	worleyNoiseCompute.setInt("numCells", cellsPerAxis);
	worleyNoiseCompute.setInt("worleyPointsBufferSize", worleyPointsSize);
	worleyNoiseCompute.setFloat("persistence", persistence);

	glDispatchCompute(worleyNoiseDimensions.x / 4, worleyNoiseDimensions.y / 4,
		worleyNoiseDimensions.z / 4);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glGenerateTextureMipmap(worleyNoiseTexture);
}
