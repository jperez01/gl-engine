#include "gl_indirect_eng.h"

void IndirectEngine::init_resources()
{
	simplePipeline = Shader("indirect/simple.vert", "indirect/simple.frag");

	directionalLight.direction = glm::vec3(0.2f, 0.4f, 0.8f);
	directionalLight.ambient = glm::vec3(0.2f);
	directionalLight.diffuse = glm::vec3(0.2f);
	directionalLight.specular = glm::vec3(0.2f);
}

void IndirectEngine::render(std::vector<Model>& objs)
{
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection = camera->getProjectionMatrix();
	glm::mat4 view = camera->getViewMatrix();
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::scale(model, glm::vec3(0.1f));

	simplePipeline.use();

	simplePipeline.setMat4("projection", projection);
	simplePipeline.setMat4("view", view);
	simplePipeline.setMat4("model", model);

	simplePipeline.setVec3("dirLight.direction", directionalLight.direction);
	simplePipeline.setVec3("dirLight.ambient", directionalLight.ambient);
	simplePipeline.setVec3("dirLight.specular", directionalLight.specular);
	simplePipeline.setVec3("dirLight.diffuse", directionalLight.diffuse);
	simplePipeline.setInt("index", index);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, indirectDrawBuffer);
	glBindVertexArray(simpleBuffer.VAO);
	glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, 0, mIndirectCommands.size(), 20);
}

void IndirectEngine::handleImGui()
{
	if (ImGui::CollapsingHeader("Directional Light")) {
		ImGui::SliderFloat3("Direction", (float*)&directionalLight.direction, -1.0, 1.0);
		ImGui::SliderFloat3("Ambient", (float*)&directionalLight.ambient, 0.0, 1.0);
		ImGui::SliderFloat3("Specular", (float*)&directionalLight.specular, 0.0, 1.0);
		ImGui::SliderFloat3("Diffuse", (float*)&directionalLight.diffuse, 0.0, 1.0);

		directionalLight.direction = glm::normalize(directionalLight.direction);
	}

	ImGui::SliderInt("Texture To Use", &index, 0, mTextureHandles.size() - 1);
}

void IndirectEngine::handleObjs(std::vector<Model>& objs)
{
	Model& model = objs.at(0);
	std::vector<Vertex> packedVertices;
	std::vector<unsigned int> packedIndices;

	unsigned int totalNumVertices = 0;
	unsigned int totalNumIndices = 0;
	for (Mesh& mesh : model.meshes) {
		totalNumVertices += mesh.vertices.size();
		totalNumIndices += mesh.indices.size();
	}

	packedVertices.reserve(totalNumVertices);
	packedIndices.reserve(totalNumIndices);

	unsigned int currentIndex = 0, currentVertex = 0, currentTexture = 0;

	for (Mesh& mesh : model.meshes) {
		IndirectCommandData data;
		data.indexCount = mesh.indices.size();
		data.instanceCount = 1;
		data.firstIndex = currentIndex;
		data.baseVertex = currentVertex;
		mIndirectCommands.push_back(data);

		packedVertices.insert(packedVertices.end(), mesh.vertices.begin(), mesh.vertices.end());
		packedIndices.insert(packedIndices.end(), mesh.indices.begin(), mesh.indices.end());

		Material& material = model.materials_loaded[mesh.materialIndex];
		for (Texture& texture : material.textures) {
			GLuint64 handle = glGetTextureHandleARB(texture.id);
			glMakeTextureHandleResidentARB(handle);

			mTextureHandles.push_back(handle);
		}
		mNumTextures.push_back(currentTexture);

		currentVertex += mesh.vertices.size();
		currentIndex += mesh.indices.size();
		currentTexture += material.textures.size();
	}

	std::vector<VertexType> endpoints = { POSITION, NORMAL, TEXCOORDS, TANGENT, BI_TANGENT, VERTEX_ID };
	simpleBuffer = glutil::loadVertexBuffer(packedVertices, packedIndices, endpoints);

	glCreateBuffers(1, &indirectDrawBuffer);
	glNamedBufferData(indirectDrawBuffer, sizeof(IndirectCommandData) * mIndirectCommands.size(),
		mIndirectCommands.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_DRAW_INDIRECT_BUFFER, 0, indirectDrawBuffer);

	glGenBuffers(1, &textureHandleBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, textureHandleBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint64) * mTextureHandles.size(), mTextureHandles.data(), GL_DYNAMIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, textureHandleBuffer);


	glCreateBuffers(1, &numTextureBuffer);
	glNamedBufferData(numTextureBuffer, sizeof(unsigned int) * mNumTextures.size(),
		mNumTextures.data(), GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, numTextureBuffer);
}
