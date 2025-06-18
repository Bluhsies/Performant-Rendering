#include "include/ui.hpp"

void UI::init(const glm::ivec2& size)
{
	// Set screen size
	m_size = size;
	
	//Reset the scene
	m_UIScene.reset(new Scene);

	// Quad shader - implements programmable vertex pulling
	ShaderDescription quadShaderDesc;
	quadShaderDesc.type = ShaderType::rasterization;
	quadShaderDesc.vertexSrcPath = "./assets/shaders/UI/QuadVert.glsl";
	quadShaderDesc.fragmentSrcPath = "./assets/shaders/UI/QuadFrag.glsl";

	std::shared_ptr<Shader> quadShader;
	quadShader = std::make_shared<Shader>(quadShaderDesc);

	// Circle shader - implements programmable vertex pulling
	ShaderDescription circleShaderDesc;
	circleShaderDesc.type = ShaderType::rasterization;
	circleShaderDesc.vertexSrcPath = "./assets/shaders/UI/CircleVert.glsl";
	circleShaderDesc.fragmentSrcPath = "./assets/shaders/UI/CircleFrag.glsl";

	std::shared_ptr<Shader> circleShader;
	circleShader = std::make_shared<Shader>(circleShaderDesc);

	// Populate indices - common to all
	std::vector<uint32_t> indices(m_maxQuadCount * 6);

	uint32_t offset{ 0 };
	for (size_t i = 0; i < indices.size(); i += 6)
	{
		indices[i + 0] = offset + 0;
		indices[i + 1] = offset + 1;
		indices[i + 2] = offset + 2;

		indices[i + 3] = offset + 2;
		indices[i + 4] = offset + 3;
		indices[i + 5] = offset + 0;

		offset += 4;
	}

	// Quads - Set up buffers, material
	std::shared_ptr<VAO> quadsVAO;
	quadsVAO = std::make_shared<VAO>(indices);

	m_quadsSSBO = std::make_shared<SSBO>(sizeof(QuadVertex) * m_quadVertices.size(), m_quadVertices.size());
	m_quadsSSBO->edit(0, sizeof(QuadVertex) * m_quadVertices.size(), m_quadVertices.data());

	std::shared_ptr<Material> quadsMaterial;
	quadsMaterial = std::make_shared<Material>(quadShader, "");

	//Set entities for the quads
	{

		m_quads = m_UIScene->m_entities.create();

		m_UIScene->m_entities.emplace<Transform>(m_quads);

		auto& renderComp = m_UIScene->m_entities.emplace<Render>(m_quads);
		renderComp.geometry = quadsVAO;
		renderComp.material = quadsMaterial;

		renderComp.material->setValue("u_textureSlots[0]", slots.data());
		renderComp.geometry->overrideDrawCount(0);

		
		auto& lodComp = m_UIScene->m_entities.emplace<LODAssign>(m_quads);
		lodComp.lodNumber = lodNonAsteroid;

	}

	// Circles - Set up buffers, material
	std::shared_ptr<VAO> circlesVAO;
	circlesVAO = std::make_shared<VAO>(indices); // Same as quads indices - circles are beounded by quads

	m_circlesSSBO = std::make_shared<SSBO>(sizeof(CircleVertex) * m_circleVertices.size(), m_circleVertices.size());
	m_circlesSSBO->edit(0, sizeof(CircleVertex) * m_circleVertices.size(), m_circleVertices.data());

	std::shared_ptr<Material> circleMaterial;
	circleMaterial = std::make_shared<Material>(circleShader, "");

	//Set entities for the circles 
	{
		m_circles = m_UIScene->m_entities.create();

		m_UIScene->m_entities.emplace<Transform>(m_circles);

		auto& renderComp = m_UIScene->m_entities.emplace<Render>(m_circles);
		renderComp.geometry = circlesVAO;
		renderComp.material = circleMaterial;
		renderComp.geometry->overrideDrawCount(0);

		auto& lodComp = m_UIScene->m_entities.emplace<LODAssign>(m_circles);
		lodComp.lodNumber = lodNonAsteroid;
	}

	/*************************
	*  UI Pass
	**************************/
	RenderPass UIpass;
	UIpass.scene = m_UIScene;
	UIpass.parseScene();
	UIpass.clearColour = false;
	UIpass.clearDepth = false;
	UIpass.target = std::make_shared<FBO>(); // Default FBO
	UIpass.viewPort = { 0, 0, static_cast<uint32_t>(m_size.x), static_cast<uint32_t>(m_size.y) };
	UIpass.camera.view = glm::mat4(1.f);
	UIpass.camera.projection = glm::ortho(0.f, static_cast<float>(m_size.x), static_cast<float>(m_size.y), 0.f);
	UIpass.UBOmanager.setCachedValue("b_cameraUI", "u_view", UIpass.camera.view);
	UIpass.UBOmanager.setCachedValue("b_cameraUI", "u_projection", UIpass.camera.projection);

	m_UIRenderer.addRenderPass(UIpass);

	// Setup default texture and sub-texture
	unsigned char whitePx[4] = { 255,255,255,255 };
	//RendererCommon::defaultTexture = resources->addTexture("defaultWhite", 1, 1, 4, whitePx);
	TextureDescription defaultTexDesc;
	defaultTexDesc.channels = 4;
	defaultTexDesc.width = 1;
	defaultTexDesc.height = 1;
	defaultTexDesc.isHDR = false;
	defaultTexDesc.data = &whitePx[0];

	m_defaultTexture = std::make_shared<Texture>(defaultTexDesc);
	//RendererCommon::defaultSubTexture = SubTexture(RendererCommon::defaultTexture, glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f));
}

void UI::onRender() const
{
	m_quadsSSBO->bind(0);
	m_circlesSSBO->bind(1);

	m_UIRenderer.render();
}

void UI::begin()
{
	// Clear texture unit mananger
	m_defaultTexture->clearTUM();

	// Clear quad data
	m_currentQuadCount = 0;
	auto& quadsRender = m_UIScene->m_entities.get<Render>(m_quads);
	quadsRender.geometry->overrideDrawCount(0);

	// Clear circle data
	m_currentCircleCount = 0;
	auto& circlesRender = m_UIScene->m_entities.get<Render>(m_circles);
	circlesRender.geometry->overrideDrawCount(0);
}

void UI::end()
{
	// Send quad vertices to the SSBO
	m_quadsSSBO->edit(0, sizeof(QuadVertex) * m_currentQuadCount * 4, m_quadVertices.data());
	// Set quad draw count
	auto& quadsRender = m_UIScene->m_entities.get<Render>(m_quads);
	quadsRender.geometry->overrideDrawCount(m_currentQuadCount * 6);

	// Send circle vertices to the SSBO
	m_circlesSSBO->edit(0, sizeof(CircleVertex) * m_currentCircleCount * 4, m_circleVertices.data());
	// Set circle draw count
	auto& circlesRender = m_UIScene->m_entities.get<Render>(m_circles);
	circlesRender.geometry->overrideDrawCount(m_currentCircleCount * 6);
}

void UI::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& colour)
{
	auto slot = m_defaultTexture->getUnit();

	size_t idx = m_currentQuadCount * 4;

	// Position
	m_quadVertices[idx + 0].position = { position.x, position.y }; 
	m_quadVertices[idx + 1].position = { position.x, position.y + size.y}; 
	m_quadVertices[idx + 2].position = { position.x + size.x, position.y + size.y };
	m_quadVertices[idx + 3].position = { position.x + size.x, position.y };
	// UVs
	m_quadVertices[idx + 0].texCoords = { 0.f, 0.f };
	m_quadVertices[idx + 1].texCoords = { 0.f, 1.f };
	m_quadVertices[idx + 2].texCoords = { 1.f, 1.f };
	m_quadVertices[idx + 3].texCoords = { 1.f, 0.f };
	// Colour
	m_quadVertices[idx + 0].colour = colour;
	m_quadVertices[idx + 1].colour = colour;
	m_quadVertices[idx + 2].colour = colour;
	m_quadVertices[idx + 3].colour = colour;
	// Slot
	m_quadVertices[idx + 0].texSlot = slot;
	m_quadVertices[idx + 1].texSlot = slot;
	m_quadVertices[idx + 2].texSlot = slot;
	m_quadVertices[idx + 3].texSlot = slot;

	m_currentQuadCount++;

	if (idx + 3 >= m_quadVertices.size()) spdlog::error("UI Quad vertices have reached limit");
}

void UI::drawTexturedQuad(const glm::vec2& position, const glm::vec2& size, std::shared_ptr<Texture> texture)
{
	auto slot = texture->getUnit();

	size_t idx = m_currentQuadCount * 4;

	// Position
	m_quadVertices[idx + 0].position = { position.x, position.y };
	m_quadVertices[idx + 1].position = { position.x, position.y + size.y };
	m_quadVertices[idx + 2].position = { position.x + size.x, position.y + size.y };
	m_quadVertices[idx + 3].position = { position.x + size.x, position.y };
	// UVs
	m_quadVertices[idx + 0].texCoords = { 0.f, 0.f };
	m_quadVertices[idx + 1].texCoords = { 0.f, 1.f };
	m_quadVertices[idx + 2].texCoords = { 1.f, 1.f };
	m_quadVertices[idx + 3].texCoords = { 1.f, 0.f };
	// Colour
	m_quadVertices[idx + 0].colour = m_defaultColour;
	m_quadVertices[idx + 1].colour = m_defaultColour;
	m_quadVertices[idx + 2].colour = m_defaultColour;
	m_quadVertices[idx + 3].colour = m_defaultColour;
	// Slot
	m_quadVertices[idx + 0].texSlot = slot;
	m_quadVertices[idx + 1].texSlot = slot;
	m_quadVertices[idx + 2].texSlot = slot;
	m_quadVertices[idx + 3].texSlot = slot;

	m_currentQuadCount++;

	if (idx + 3 >= m_quadVertices.size()) spdlog::error("UI Quad vertices have reached limit");
}

void UI::drawCircle(const glm::vec2& centre, float radius, const glm::vec4& colour, float thickness)
{

	size_t idx = m_currentCircleCount * 4;

	// Position
	m_circleVertices[idx + 0].position = { centre.x - radius, centre.y - radius};
	m_circleVertices[idx + 1].position = { centre.x - radius, centre.y + radius };
	m_circleVertices[idx + 2].position = { centre.x + radius, centre.y + radius };
	m_circleVertices[idx + 3].position = { centre.x + radius, centre.y - radius };
	// Unit position
	m_circleVertices[idx + 0].unitPosition = { -1.f, -1.f };
	m_circleVertices[idx + 1].unitPosition = { -1.f,  1.f };
	m_circleVertices[idx + 2].unitPosition = {  1.f,  1.f };
	m_circleVertices[idx + 3].unitPosition = {  1.f, -1.f };
	// Colour
	m_circleVertices[idx + 0].colour = colour;
	m_circleVertices[idx + 1].colour = colour;
	m_circleVertices[idx + 2].colour = colour;
	m_circleVertices[idx + 3].colour = colour;
	// Thickness
	m_circleVertices[idx + 0].thickness = thickness;
	m_circleVertices[idx + 1].thickness = thickness;
	m_circleVertices[idx + 2].thickness = thickness;
	m_circleVertices[idx + 3].thickness = thickness;

	m_currentCircleCount++;

	if (idx + 3 >= m_circleVertices.size()) spdlog::error("UI Circle vertices have reached limit");
}
