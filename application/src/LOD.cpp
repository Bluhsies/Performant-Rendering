#include "LOD.hpp"
#include "scripts/include/rotation.hpp"
#include "meshoptimizer.h"

LOD::LOD(GLFWWindowImpl& win) : Layer(win)
{

	m_mainScene.reset(new Scene);

	DirectionalLight dl;
	dl.direction = glm::normalize(glm::vec3(1.f, -2.5f, -3.5f));
	m_mainScene->m_directionalLights.push_back(dl);

	ShaderDescription pbrShaderDesc;
	pbrShaderDesc.type = ShaderType::rasterization;
	pbrShaderDesc.vertexSrcPath = "./assets/shaders/PBR/pbrVertex.glsl";
	pbrShaderDesc.fragmentSrcPath = "./assets/shaders/PBR/pbrFrag.glsl";

	std::shared_ptr<Shader> pbrShader;
	pbrShader = std::make_shared<Shader>(pbrShaderDesc);

	VBOLayout modelLayout = {
				{GL_FLOAT, 3},
				{GL_FLOAT, 3},
				{GL_FLOAT, 2},
				{GL_FLOAT, 3}
	};

	uint32_t attributeTypes = Model::VertexFlags::positions |
		Model::VertexFlags::normals |
		Model::VertexFlags::uvs |
		Model::VertexFlags::tangents;

	std::array<std::shared_ptr<VAO>, 5> asteroidVAOs;
	std::shared_ptr<Material> asteroidMaterials;

	//	Asteroid Model
	std::shared_ptr<Texture> asteroid_albedo = std::make_shared<Texture>("./assets/models/asteroid1/albedo.jpg");
	std::shared_ptr<Texture> asteroid_normal = std::make_shared<Texture>("./assets/models/asteroid1/normal.jpg");
	std::shared_ptr<Texture> asteroid_metal = std::make_shared<Texture>("./assets/models/asteroid1/metallic.jpg");
	std::shared_ptr<Texture> asteroid_rough = std::make_shared<Texture>("./assets/models/asteroid1/roughness.jpg");
	std::shared_ptr<Texture> asteroid_AO = std::make_shared<Texture>("./assets/models/asteroid1/AO.png");

	asteroidMaterials = std::make_shared<Material>(pbrShader);
	asteroidMaterials->setValue("albedoTexture", asteroid_albedo);
	asteroidMaterials->setValue("normalTexture", asteroid_normal);
	asteroidMaterials->setValue("roughTexture", asteroid_rough);
	asteroidMaterials->setValue("metalTexture", asteroid_metal);
	asteroidMaterials->setValue("aoTexture", asteroid_AO);

	Model asteroidModel("./assets/models/asteroid1/asteroid.obj", attributeTypes);

	asteroidVAOs[0] = std::make_shared<VAO>(asteroidModel.m_meshes[0].indices);
	asteroidVAOs[0]->addVertexBuffer(asteroidModel.m_meshes[0].vertices, modelLayout);

	{
		entt::entity raw = m_mainScene->m_entities.create();

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(raw);
		renderComp.geometry = asteroidVAOs[0];
		renderComp.material = asteroidMaterials;

		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(raw);
		transformComp.translation = glm::vec3(-2.f, 2.f, -6.f);
		transformComp.scale = glm::vec3(1.0);
		transformComp.recalc();

		auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(raw);
		scriptComp.attachScript<RotationScript>(raw, m_mainScene, glm::vec3(0.4f, 0.25f, -0.6f), GLFW_KEY_SPACE);
	}

	const size_t vertexCountOnLoad = asteroidModel.m_meshes[0].vertices.size() / vertexComponents;
	const size_t indexCountOnLoad = asteroidModel.m_meshes[0].indices.size();

	{

		std::vector<uint32_t> remap(asteroidModel.m_meshes[0].vertices.size()); // Needs to take in vertices rather than indices, as we need vertex_count elements not indices.

		const size_t vertexCount = meshopt_generateVertexRemap(
			remap.data(),
			asteroidModel.m_meshes[0].indices.data(),
			indexCountOnLoad,
			asteroidModel.m_meshes[0].vertices.data(),
			vertexCountOnLoad,
			vertexStride
		);

		std::vector<uint32_t> remappedIndices(asteroidModel.m_meshes[0].indices.size());
		std::vector<float> remappedVertices(vertexCount * vertexComponents);

		meshopt_remapIndexBuffer(
			remappedIndices.data(),
			asteroidModel.m_meshes[0].indices.data(),
			indexCountOnLoad,
			remap.data()
		);

		meshopt_remapVertexBuffer(

			remappedVertices.data(),
			asteroidModel.m_meshes[0].vertices.data(),
			vertexCountOnLoad,
			vertexStride,
			remap.data()

		);

		meshopt_optimizeVertexCache(
			remappedIndices.data(),
			remappedIndices.data(),
			indexCountOnLoad,
			vertexCount
		); // Reduces the number of GPU vertex shader invocations. Takes in remappedIndices.data as the destination and the indices, as we want to reorder these
		//to be more efficient.

		meshopt_optimizeOverdraw(
			remappedIndices.data(),
			remappedIndices.data(),
			indexCountOnLoad,
			remappedVertices.data(),
			vertexCount,
			vertexStride,
			1.05f
		); //Reduces the number of GPU vertex shader invocations and pixel overdraw.

		meshopt_optimizeVertexFetch(
			remappedVertices.data(),
			remappedIndices.data(),
			indexCountOnLoad,
			remappedVertices.data(),
			vertexCount,
			vertexStride
		); //Reorders vertices and changes indices to reduce the amount of GPU memory fetches during vertex processing. 
		//Returns the number of unique vertices, which is the same as vertex count unless some vertices are unused.

		const float threshold1 = 0.5f; //LOD1 threshold, the lower the number, the fewer vertices there are.
		const float threshold2 = 0.25f; //LOD2 threshold
		
		size_t LOD1_target_index_count = size_t(remappedIndices.size() * threshold1); // Ideal LOD1 index count
		size_t LOD2_target_index_count = size_t(remappedIndices.size() * threshold2); // Ideal LOD2 index count
		
		const float LOD1_target_error = 0.01f; // Error allowed, the higher the number the greater the deviation allowed.
		const float LOD2_target_error = 0.02f; // Error allowed for LOD2
		

		m_indicesLOD1.resize(remappedIndices.size());

		uint32_t LOD1IndexCount = meshopt_simplify(
			m_indicesLOD1.data(),
			remappedIndices.data(),
			remappedIndices.size(),
			remappedVertices.data(),
			vertexCount,
			vertexStride,
			LOD1_target_index_count,
			LOD1_target_error
		);

		m_indicesLOD2.resize(remappedIndices.size());

		uint32_t LOD2IndexCount = meshopt_simplify(
			m_indicesLOD2.data(),
			remappedIndices.data(),
			remappedIndices.size(),
			remappedVertices.data(),
			vertexCount,
			vertexStride,
			LOD2_target_index_count,
			LOD2_target_error
		);

		

		m_indices = remappedIndices;
		m_vertices = remappedVertices;
		m_indicesLOD1.erase(m_indicesLOD1.begin() + LOD1IndexCount, m_indicesLOD1.end()); // Cut down to actual size
		m_indicesLOD2.erase(m_indicesLOD2.begin() + LOD2IndexCount, m_indicesLOD2.end()); // Cut down to actual size
		

		spdlog::info("Verts on load: {} indices on load: {} ", vertexCountOnLoad, indexCountOnLoad);
		spdlog::info("Optimised verts: {} indices: {} ", remappedVertices.size() / vertexComponents, remappedIndices.size());
		spdlog::info("LOD1 target index count {}  actual index count {}", LOD1_target_index_count, LOD1IndexCount);
		spdlog::info("LOD2 target index count {}  actual index count {}", LOD2_target_index_count, LOD2IndexCount);
		

	}

	asteroidVAOs[1] = std::make_shared<VAO>(m_indices);
	asteroidVAOs[1]->addVertexBuffer(m_vertices, modelLayout);

	{

		entt::entity optimised = m_mainScene->m_entities.create();

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(optimised);
		renderComp.geometry = asteroidVAOs[1];
		renderComp.material = asteroidMaterials;

		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(optimised);
		transformComp.translation = glm::vec3(2.f, 2.f, -6.f);
		transformComp.scale = glm::vec3(1.0);
		transformComp.recalc();

		auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(optimised);
		scriptComp.attachScript<RotationScript>(optimised, m_mainScene, glm::vec3(0.4f, 0.25f, -0.6f), GLFW_KEY_SPACE);

	}

	asteroidVAOs[2] = std::make_shared<VAO>(m_indicesLOD1);
	asteroidVAOs[2]->addVertexBuffer(m_vertices, modelLayout);
	{
		entt::entity LOD1 = m_mainScene->m_entities.create();

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(LOD1);
		renderComp.geometry = asteroidVAOs[2];
		renderComp.material = asteroidMaterials;

		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(LOD1);
		transformComp.translation = glm::vec3(-2.f, -2.f, -6.f);
		transformComp.scale = glm::vec3(1.0);
		transformComp.recalc();

		auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(LOD1);
		scriptComp.attachScript<RotationScript>(LOD1, m_mainScene, glm::vec3(0.4f, 0.25f, -0.6f), GLFW_KEY_SPACE);
	}

	asteroidVAOs[3] = std::make_shared<VAO>(m_indicesLOD2);
	asteroidVAOs[3]->addVertexBuffer(m_vertices, modelLayout);
	{
		entt::entity LOD2 = m_mainScene->m_entities.create();

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(LOD2);
		renderComp.geometry = asteroidVAOs[3];
		renderComp.material = asteroidMaterials;

		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(LOD2);
		transformComp.translation = glm::vec3(2.f, -2.f, -6.f);
		transformComp.scale = glm::vec3(1.0);
		transformComp.recalc();

		auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(LOD2);
		scriptComp.attachScript<RotationScript>(LOD2, m_mainScene, glm::vec3(0.4f, 0.25f, -0.6f), GLFW_KEY_SPACE);
	}

	asteroidVAOs[4] = std::make_shared<VAO>(m_indicesLOD3);
	asteroidVAOs[4]->addVertexBuffer(m_vertices, modelLayout);
	{
		entt::entity LOD3 = m_mainScene->m_entities.create();

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(LOD3);
		renderComp.geometry = asteroidVAOs[4];
		renderComp.material = asteroidMaterials;

		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(LOD3);
		transformComp.translation = glm::vec3(0.f, 0.f, -6.f);
		transformComp.scale = glm::vec3(1.0);
		transformComp.recalc();

		auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(LOD3);
		scriptComp.attachScript<RotationScript>(LOD3, m_mainScene, glm::vec3(0.4f, 0.25f, -0.6f), GLFW_KEY_SPACE);
	}

	std::vector<uint32_t> allIndices;
	allIndices.insert(allIndices.end(), m_indices.begin(), m_indices.end());
	allIndices.insert(allIndices.end(), m_indicesLOD1.begin(), m_indicesLOD1.end());
	allIndices.insert(allIndices.end(), m_indicesLOD2.begin(), m_indicesLOD2.end());
	allIndices.insert(allIndices.end(), m_indicesLOD3.begin(), m_indicesLOD3.end());

	std::shared_ptr<VAO> allLODsVAO = std::make_shared<VAO>(allIndices);
	allLODsVAO->addVertexBuffer(m_vertices, modelLayout);

	{

		allLODs = m_mainScene->m_entities.create();

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(allLODs);
		renderComp.geometry = allLODsVAO;
		renderComp.material = asteroidMaterials;

		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(allLODs);
		transformComp.translation = glm::vec3(0.f, 0.f, -6.f);
		transformComp.scale = glm::vec3(1.0);
		transformComp.recalc();

		auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(allLODs);
		scriptComp.attachScript<RotationScript>(allLODs, m_mainScene, glm::vec3(0.4f, 0.25f, -0.6f), GLFW_KEY_SPACE);

	}

	mainPass.scene = m_mainScene;
	mainPass.parseScene();
	mainPass.target = std::make_shared<FBO>();
	mainPass.camera.projection = glm::perspective(45.f, m_winRef.getWidthf() / m_winRef.getHeightf(), 0.1f, 2000.f);
	mainPass.viewPort = { 0,0,m_winRef.getWidth(), m_winRef.getHeight() };

	mainPass.UBOmanager.setCachedValue("b_camera", "u_view", mainPass.camera.view);
	mainPass.UBOmanager.setCachedValue("b_camera", "u_projection", mainPass.camera.projection);
	mainPass.UBOmanager.setCachedValue("b_camera", "u_viewPos", glm::vec3(0.f));

	mainPass.UBOmanager.setCachedValue("b_lights", "dLight.colour", m_mainScene->m_directionalLights.at(0).colour);
	mainPass.UBOmanager.setCachedValue("b_lights", "dLight.direction", m_mainScene->m_directionalLights.at(0).direction);

	m_mainRenderer.addRenderPass(mainPass);

	glClearColor(1.f, 0.f, 1.f, 1.f);

}

void LOD::onRender()
{

	m_mainRenderer.render();

}

void LOD::onUpdate(float timestep)
{

	timestep = std::clamp(timestep, 0.f, 0.1f); // Clamp to be no more than a 10th of a second for physics
	// Update scripts
	auto view = m_mainScene->m_entities.view<ScriptComp>();
	for (auto entity : view)
	{
		auto& script = view.get<ScriptComp>(entity);
		script.onUpdate(timestep);
	}

}

void LOD::onKeyPressed(KeyPressedEvent& e)
{

	if (e.getKeyCode() == GLFW_KEY_W)
	{
		if (m_wireframe) {
			m_wireframe = false;
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else {
			m_wireframe = true;
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
	}

}

void LOD::onImGUIRender()
{

	// Scripts
	auto view = m_mainScene->m_entities.view<ScriptComp>();
	for (auto entity : view)
	{
		auto& script = view.get<ScriptComp>(entity);
		script.onImGuiRender();
	}

}
