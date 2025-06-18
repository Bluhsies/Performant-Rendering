#include "GAMR3531.hpp"
#include <numeric> // For std::iota
#include <glm/gtc/matrix_transform.hpp>
#include "core/randomiser.hpp" // For random number generation
#include "tracy/Tracy.hpp"
#include "tracy/TracyOpenGL.hpp"
#include "scripts/include/controller.hpp"
#include "scripts/include/rotation.hpp"
#include "meshoptimizer.h"
#include <iostream>


AsteriodBelt::AsteriodBelt(GLFWWindowImpl& win) : Layer(win)
{
	ZoneScopedN("Main");
	TracyGpuZone("Main");

	m_closeAsteroids.reserve(20);

	m_mainScene.reset(new Scene);

	generateLevel();

	m_broadPhase.init(m_mainScene); // Call the broad phase init function to setup the AABBs

	/*************************
	*  Main Render Pass
	**************************/

	RenderPass mainPass;
	FBOLayout typicalLayout = {
		{AttachmentType::ColourHDR, true},
		{AttachmentType::Depth, false}
	};

	mainPass.scene = m_mainScene;
	mainPass.parseScene();
	mainPass.target = std::make_shared<FBO>(m_winRef.getSize(), typicalLayout);
	mainPass.camera.projection = glm::perspective(45.f, m_winRef.getWidthf() / m_winRef.getHeightf(), 0.1f, 2000.f);
	mainPass.viewPort = { 0, 0, m_winRef.getWidth(), m_winRef.getHeight() };

	mainPass.camera.updateView(m_mainScene->m_entities.get<Transform>(camera).transform);

	mainPass.UBOmanager.setCachedValue("b_camera", "u_view", mainPass.camera.view);
	mainPass.UBOmanager.setCachedValue("b_camera", "u_projection", mainPass.camera.projection);
	mainPass.UBOmanager.setCachedValue("b_camera", "u_viewPos", m_mainScene->m_entities.get<Transform>(camera).translation);

	mainPass.UBOmanager.setCachedValue("b_lights", "dLight.colour", m_mainScene->m_directionalLights.at(0).colour);
	mainPass.UBOmanager.setCachedValue("b_lights", "dLight.direction", m_mainScene->m_directionalLights.at(0).direction);


	m_mainRenderer.addRenderPass(mainPass);

	/*************************
	*  Bloom
	**************************/

	// General setup

	// Values common across all passes
	const std::vector<uint32_t> screenIndices = { 0,1,2,2,3,0 };

	VBOLayout screenQuadLayout = {
		{GL_FLOAT, 3},
		{GL_FLOAT, 2}
	};

	FBOLayout screenLayout = {
		{AttachmentType::ColourHDR, true}
	};

	// Bloom Threshold pass
	auto thresholdPassIdx = m_mainRenderer.getRenderPassCount();
	RenderPass thresholdPass;

	ShaderDescription thresholdShaderDesc;
	thresholdShaderDesc.type = ShaderType::rasterization;
	thresholdShaderDesc.vertexSrcPath = "./assets/shaders/Bloom/Vert.glsl";
	thresholdShaderDesc.fragmentSrcPath = "./assets/shaders/Bloom/ThresholdFrag.glsl";

	std::shared_ptr<Shader> thresholdShader;
	thresholdShader = std::make_shared<Shader>(thresholdShaderDesc);

	std::shared_ptr<Material> thresholdMaterial;
	thresholdMaterial = std::make_shared<Material>(thresholdShader, "");

	thresholdPass.camera.projection = glm::ortho(0.f, m_winRef.getWidthf(), m_winRef.getHeightf(), 0.f);
	thresholdMaterial->setValue("u_projection", thresholdPass.camera.projection);
	thresholdMaterial->setValue("u_albedoMap", mainPass.target->getTarget(0));
	thresholdMaterial->setValue("u_threshold", m_bloomPanel.getThreshold());

	std::shared_ptr<Scene> thresholdScene = std::make_shared<Scene>();

	std::vector<float> screenVertices = {
		// Position											UV
		0.f, 0.f, 0.f,										0.f, 1.f,
		m_winRef.getWidthf(),	0.f, 0.f,					1.f, 1.f,
		m_winRef.getWidthf(), m_winRef.getHeightf(), 0.f,	1.f, 0.f,
		0.f, m_winRef.getHeightf(), 0.f,					0.f, 0.f,
	};

	std::shared_ptr<VAO> screenVAO = std::make_shared<VAO>(screenIndices);
	screenVAO->addVertexBuffer(screenVertices, screenQuadLayout);


	// Scoped quad so that it is limited and can't be used outside of the curly braces.
	{
		// Create the entity in the scene.
		entt::entity quad = thresholdScene->m_entities.create();

		//Add a render component and keep a reference to it in this scope.
		auto& renderComp = thresholdScene->m_entities.emplace<Render>(quad);
		// Set the geometry and material.
		renderComp.geometry = screenVAO;
		renderComp.material = thresholdMaterial;
		//Add the transform component.
		thresholdScene->m_entities.emplace<Transform>(quad);

		auto& lodComp = thresholdScene->m_entities.emplace<LODAssign>(quad);
		lodComp.lodNumber = lodNonAsteroid;

	}

	glm::mat4 screenProj = glm::ortho(0.f, m_winRef.getWidthf(), m_winRef.getHeightf(), 0.f);
	ViewPort screenViewport = { 0, 0, m_winRef.getWidth(), m_winRef.getHeight() };

	std::shared_ptr<FBO>screenFBO = std::make_shared<FBO>(m_winRef.getSize(), screenLayout);

	thresholdPass.scene = thresholdScene;
	thresholdPass.parseScene();
	thresholdPass.target = screenFBO;
	thresholdPass.viewPort = screenViewport;
	thresholdPass.camera.projection = screenProj;

	m_mainRenderer.addRenderPass(thresholdPass);

	const int downScalePasses = 6; // Number of downscale passes
	const int upScalePasses = 5; // Number of upscale passes

	// VAOs for each set of vertices
	std::array<std::shared_ptr<VAO>, downScalePasses> screenVAOs;

	// Projection matrices, ortho
	std::array<glm::mat4, downScalePasses> screenProjs;

	// Viewports
	std::array<ViewPort, downScalePasses> screenViewports;

	// Scenes
	std::array<Scene, downScalePasses> screenScenes;

	// FBOs (up and down)
	std::array<std::shared_ptr<FBO>, downScalePasses + upScalePasses> screenFBOs;

	// UV per pixel
	std::array<glm::vec2, downScalePasses> UVperPixels;

	float divisor = 2.f;
	for (size_t i = 0; i < downScalePasses; i++) {
		const float w = ceil(m_winRef.getWidthf() / divisor);
		const float h = ceil(m_winRef.getHeightf() / divisor);
		const uint32_t w_i = static_cast<uint32_t>(w);
		const uint32_t h_i = static_cast<uint32_t>(h);

		std::vector<float> screenVertices = {
			// Position		UV
			0.f, 0.f, 0.f,	0.f, 1.f,
			w,	0.f, 0.f,	1.f, 1.f,
			w, h, 0.f,		1.f, 0.f,
			0.f, h, 0.f,	0.f, 0.f,
		};

		screenVAOs[i] = std::make_shared<VAO>(screenIndices);
		screenVAOs[i]->addVertexBuffer(screenVertices, screenQuadLayout);

		screenProjs[i] = glm::ortho(0.f, w, h, 0.f);
		screenViewports[i] = { 0, 0, w_i, h_i };
		screenFBOs[i] = std::make_shared<FBO>(glm::ivec2(w_i, h_i), screenLayout);

		UVperPixels[i] = { 1.f / w, 1.f / h };

		divisor += divisor;
	}

	// FBOs for upscale passes
	divisor /= 4.f;
	for (size_t i = downScalePasses; i < downScalePasses + upScalePasses; i++) {
		const uint32_t w_i = static_cast<uint32_t>(ceil(m_winRef.getWidthf() / divisor));
		const uint32_t h_i = static_cast<uint32_t>(ceil(m_winRef.getHeightf() / divisor));
		screenFBOs[i] = std::make_shared<FBO>(glm::ivec2(w_i, h_i), screenLayout);
		divisor /= 2.f;
	}

	// Downscale and blur passes (6 in total)

	ShaderDescription downBlurShaderDesc;
	downBlurShaderDesc.type = ShaderType::rasterization;
	downBlurShaderDesc.vertexSrcPath = "./assets/shaders/Bloom/Vert.glsl";
	downBlurShaderDesc.fragmentSrcPath = "./assets/shaders/Bloom/DownBlurFrag.glsl";

	std::shared_ptr<Shader> downBlurShader;
	downBlurShader = std::make_shared<Shader>(downBlurShaderDesc);

	std::array<std::shared_ptr<Material>, downScalePasses> downBlurMaterials;

	for (size_t i = 0; i < downScalePasses; i++) {
		RenderPass downBlurPass;

		downBlurPass.camera.projection = screenProjs[i];
		downBlurMaterials[i] = std::make_shared<Material>(downBlurShader, "");
		auto& downBlurMaterial = downBlurMaterials[i];
		downBlurMaterial->setValue("u_projection", downBlurPass.camera.projection);
		downBlurMaterial->setValue("u_albedoMap", thresholdPass.target->getTarget(0));
		downBlurMaterial->setValue("u_UVperPixel", UVperPixels[i]);

		m_bloomScenes.push_back(std::make_shared<Scene>());

		

		entt::entity quad = m_bloomScenes.back()->m_entities.create();

		auto& renderComp = m_bloomScenes.back()->m_entities.emplace<Render>(quad);

		renderComp.geometry = screenVAOs[i];
		renderComp.material = downBlurMaterial;

		
		auto& lodComp = m_bloomScenes.back()->m_entities.emplace<LODAssign>(quad);
		lodComp.lodNumber = lodNonAsteroid;

		m_bloomScenes.back()->m_entities.emplace<Transform>(quad);

		downBlurPass.scene = m_bloomScenes.back();
		downBlurPass.parseScene();
		downBlurPass.target = screenFBOs[i];
		downBlurPass.viewPort = screenViewports[i];
		downBlurPass.camera.projection = screenProjs[i];

		m_mainRenderer.addRenderPass(downBlurPass);
	}

	// Upscale and Karim filter passes (5 in total)

	ShaderDescription upFilterShaderDesc;
	upFilterShaderDesc.type = ShaderType::rasterization;
	upFilterShaderDesc.vertexSrcPath = "./assets/shaders/Bloom/Vert.glsl";
	upFilterShaderDesc.fragmentSrcPath = "./assets/shaders/Bloom/UpFilterFrag.glsl";

	std::shared_ptr<Shader> upFilterShader;
	upFilterShader = std::make_shared<Shader>(upFilterShaderDesc);

	std::array<std::shared_ptr<Material>, upScalePasses> upFilterMaterials;

	for (size_t i = 0; i < upScalePasses; i++) {
		RenderPass upFilterPass;

		size_t toAddToIdx = downScalePasses - 2 - i; // Index of to add to data, initially data from the penultimate down scale pass, think of this as current target
		size_t toFilterToIdx = downScalePasses - 1 - i; // Index of to add to filter, initially data from the final scale pass, think of this as prevoius pass data to be filtered in

		upFilterPass.camera.projection = screenProjs[toAddToIdx];
		upFilterMaterials[i] = std::make_shared<Material>(upFilterShader, "");
		auto& upFilterMaterial = upFilterMaterials[i];
		upFilterMaterial->setValue("u_projection", upFilterPass.camera.projection);
		std::shared_ptr<Texture> toFilterTexture = m_mainRenderer.getRenderPass(thresholdPassIdx + downScalePasses - i).target->getTarget(0);
		upFilterMaterial->setValue("u_toFilter", toFilterTexture);
		std::shared_ptr<Texture> toAddToTexture = m_mainRenderer.getRenderPass(thresholdPassIdx + downScalePasses - (i + 1)).target->getTarget(0);
		upFilterMaterial->setValue("u_toAddTo", toAddToTexture);
		upFilterMaterial->setValue("u_filterUVperPixel", UVperPixels[toFilterToIdx]);
		upFilterMaterial->setValue("u_filterScalar", 1.f / static_cast<float>(upScalePasses));
		if (i == 0) upFilterMaterial->setValue("u_addToScalar", 1.f / static_cast<float>(upScalePasses));
		else upFilterMaterial->setValue("u_addToScalar", 1.f);

		m_bloomScenes.push_back(std::make_shared<Scene>());

		

		entt::entity quad = m_bloomScenes.back()->m_entities.create();

		auto& renderComp = m_bloomScenes.back()->m_entities.emplace<Render>(quad);

		renderComp.geometry = screenVAOs[toAddToIdx];
		renderComp.material = upFilterMaterial;

		
		auto& lodComp = m_bloomScenes.back()->m_entities.emplace<LODAssign>(quad);
		lodComp.lodNumber = lodNonAsteroid;

		m_bloomScenes.back()->m_entities.emplace<Transform>(quad);

		upFilterPass.clearColour = false;
		upFilterPass.clearDepth = false;
		upFilterPass.scene = m_bloomScenes.back();
		upFilterPass.parseScene();
		upFilterPass.target = screenFBOs[downScalePasses + i];
		upFilterPass.viewPort = screenViewports[toAddToIdx];
		upFilterPass.camera.projection = screenProjs[toAddToIdx];

		m_mainRenderer.addRenderPass(upFilterPass);
	}

	/*************************
	*  Screen Pass
	**************************/

	ShaderDescription screenShaderDesc;
	screenShaderDesc.type = ShaderType::rasterization;
	screenShaderDesc.vertexSrcPath = "./assets/shaders/Composition/Vert.glsl";
	screenShaderDesc.fragmentSrcPath = "./assets/shaders/Composition/Frag.glsl";

	std::shared_ptr<Shader> screenShader;
	screenShader = std::make_shared<Shader>(screenShaderDesc);

	std::shared_ptr<Material> screenQuadMaterial;
	screenQuadMaterial = std::make_shared<Material>(screenShader);
	screenQuadMaterial->setValue("u_albedoMap", mainPass.target->getTarget(0));
	std::shared_ptr<Texture> bloomTexture = m_mainRenderer.getRenderPass(thresholdPassIdx + downScalePasses + upScalePasses).target->getTarget(0);
	screenQuadMaterial->setValue("u_bloomMap", bloomTexture);

	m_screenScene.reset(new Scene);

	{
		entt::entity quad = m_screenScene->m_entities.create();

		auto& renderComp = m_screenScene->m_entities.emplace<Render>(quad);

		renderComp.geometry = screenVAO;
		renderComp.material = screenQuadMaterial;

		
		auto& lodComp = m_screenScene->m_entities.emplace<LODAssign>(quad);
		lodComp.lodNumber = lodNonAsteroid;

		m_screenScene->m_entities.emplace<Transform>(quad);

	}

	RenderPass screenPass;
	screenPass.scene = m_screenScene;
	screenPass.parseScene();
	screenPass.target = std::make_shared<FBO>(); // Default FBO
	screenPass.viewPort = screenViewport;  // Same as Bloom threshold pass
	screenPass.camera.projection = screenProj; // Same as Bloom threshold pass
	screenPass.UBOmanager.setCachedValue("b_camera2D", "u_view", screenPass.camera.view);
	screenPass.UBOmanager.setCachedValue("b_camera2D", "u_projection", screenPass.camera.projection);

	m_mainRenderer.addRenderPass(screenPass);

	// UI
	m_ui.init(m_winRef.getSize());
	m_introTexture.reset(new Texture("./assets/textures/UI/intro.png"));
	m_gameOverTexture.reset(new Texture("./assets/textures/UI/gameOver.png"));

}

void AsteriodBelt::onRender()
{
	ZoneScopedN("OnRender");

	m_mainRenderer.render();
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	// Draw UI
	m_ui.begin();
	{
		ZoneScopedN("UIBegin");
		if (m_state == GameState::intro) m_ui.drawTexturedQuad({ 0.f, 0.f }, m_winRef.getSizef(), m_introTexture);
	}
	if (m_state == GameState::gameOver) m_ui.drawTexturedQuad({ 0.f, 0.f }, m_winRef.getSizef(), m_gameOverTexture);

	if (m_state == GameState::running) {
		// Speed widget

		glm::vec4 noColor(0.58, 0.573, 0.678, 0.60);
		// Quads
		m_ui.drawQuad({ 1015.23429376605f, 677.060052230405f }, { 15.f, 50.9399477695946f }, m_speedUIColours[0]);
		m_ui.drawQuad({ 1036.10357597866f, 676.183738511636f }, { 15.f, 51.8162614883643f }, speed < m_speedThresholds[0] ? m_speedUIColours[1] : noColor);
		m_ui.drawQuad({ 1056.97285819127f, 675.142292174748f }, { 15.f, 52.8577078252518f }, speed < m_speedThresholds[1] ? m_speedUIColours[2] : noColor);
		{
			ZoneScopedN("FourthQuadDraw");
			m_ui.drawQuad({ 1077.84214040388f, 673.934247237929f }, { 15.f, 54.065752762071f }, speed < m_speedThresholds[2] ? m_speedUIColours[3] : noColor);
		}
		m_ui.drawQuad({ 1098.71142261648f, 672.557881377036f }, { 15.f, 55.4421186229637f }, speed < m_speedThresholds[3] ? m_speedUIColours[4] : noColor);
		m_ui.drawQuad({ 1119.58070482909f, 671.011203218924f }, { 15.f, 56.9887967810764f }, speed < m_speedThresholds[4] ? m_speedUIColours[5] : noColor);
		m_ui.drawQuad({ 1140.4499870417f,  669.291937293714f }, { 15.f, 58.7080627062856f }, speed < m_speedThresholds[5] ? m_speedUIColours[6] : noColor);
		m_ui.drawQuad({ 1161.31926925431f, 667.397506369484f }, { 15.f, 60.602493630516f }, speed < m_speedThresholds[6] ? m_speedUIColours[7] : noColor);
		m_ui.drawQuad({ 1182.18855146691f, 665.325010832928f }, { 15.f, 62.6749891670719f }, speed < m_speedThresholds[7] ? m_speedUIColours[8] : noColor);
		m_ui.drawQuad({ 1203.05783367952f, 663.071204707666f }, { 15.f, 64.9287952923339f }, speed < m_speedThresholds[8] ? m_speedUIColours[9] : noColor);
		m_ui.drawQuad({ 1223.92711589213f, 660.632467814566f }, { 15.f, 67.3675321854339f }, speed < m_speedThresholds[9] ? m_speedUIColours[10] : noColor);
		m_ui.drawQuad({ 1244.79639810473f, 658.004773471693f }, { 15.f, 69.9952265283073f }, speed < m_speedThresholds[10] ? m_speedUIColours[11] : noColor);
		m_ui.drawQuad({ 1265.66568031734f, 655.183650999685f }, { 15.f, 72.8163490003149f }, speed < m_speedThresholds[11] ? m_speedUIColours[12] : noColor);
		m_ui.drawQuad({ 1286.53496252995f, 652.164142134382f }, { 15.f, 75.8358578656179f }, speed < m_speedThresholds[12] ? m_speedUIColours[13] : noColor);
		m_ui.drawQuad({ 1307.40424474256f, 648.940750242794f }, { 15.f, 79.0592497572063f }, speed < m_speedThresholds[13] ? m_speedUIColours[14] : noColor);
		m_ui.drawQuad({ 1328.27352695516f, 645.50738097831f }, { 15.f, 82.4926190216903f }, speed < m_speedThresholds[14] ? m_speedUIColours[15] : noColor);

		// Bottom Circles														
		m_ui.drawCircle({ 1022.73429376605f, 728.f }, { 7.5f }, m_speedUIColours[0], 1.f);
		m_ui.drawCircle({ 1043.60357597866f, 728.f }, { 7.5f }, speed < m_speedThresholds[0] ? m_speedUIColours[1] : noColor, 1.f);
		m_ui.drawCircle({ 1064.47285819127f, 728.f }, { 7.5f }, speed < m_speedThresholds[1] ? m_speedUIColours[2] : noColor, 1.f);
		m_ui.drawCircle({ 1085.34214040388f, 728.f }, { 7.5f }, speed < m_speedThresholds[2] ? m_speedUIColours[3] : noColor, 1.f);
		m_ui.drawCircle({ 1106.21142261648f, 728.f }, { 7.5f }, speed < m_speedThresholds[3] ? m_speedUIColours[4] : noColor, 1.f);
		m_ui.drawCircle({ 1127.08070482909f, 728.f }, { 7.5f }, speed < m_speedThresholds[4] ? m_speedUIColours[5] : noColor, 1.f);
		m_ui.drawCircle({ 1147.9499870417f, 728.f }, { 7.5f }, speed < m_speedThresholds[5] ? m_speedUIColours[6] : noColor, 1.f);
		m_ui.drawCircle({ 1168.81926925431f, 728.f }, { 7.5f }, speed < m_speedThresholds[6] ? m_speedUIColours[7] : noColor, 1.f);
		m_ui.drawCircle({ 1189.68855146691f, 728.f }, { 7.5f }, speed < m_speedThresholds[7] ? m_speedUIColours[8] : noColor, 1.f);
		{
			ZoneScopedN("TenthBottomCircleDraw");
			m_ui.drawCircle({ 1210.55783367952f, 728.f }, { 7.5f }, speed < m_speedThresholds[8] ? m_speedUIColours[9] : noColor, 1.f);
		}
		m_ui.drawCircle({ 1231.42711589213f, 728.f }, { 7.5f }, speed < m_speedThresholds[9] ? m_speedUIColours[10] : noColor, 1.f);
		m_ui.drawCircle({ 1252.29639810473f, 728.f }, { 7.5f }, speed < m_speedThresholds[10] ? m_speedUIColours[11] : noColor, 1.f);
		m_ui.drawCircle({ 1273.16568031734f, 728.f }, { 7.5f }, speed < m_speedThresholds[11] ? m_speedUIColours[12] : noColor, 1.f);
		m_ui.drawCircle({ 1294.03496252995f, 728.f }, { 7.5f }, speed < m_speedThresholds[12] ? m_speedUIColours[13] : noColor, 1.f);
		m_ui.drawCircle({ 1314.90424474256f, 728.f }, { 7.5f }, speed < m_speedThresholds[13] ? m_speedUIColours[14] : noColor, 1.f);
		m_ui.drawCircle({ 1335.77352695516f, 728.f }, { 7.5f }, speed < m_speedThresholds[14] ? m_speedUIColours[15] : noColor, 1.f);

		// Top circles
		m_ui.drawCircle({ 1022.73429376605f, 677.060052230405f }, { 7.5f }, m_speedUIColours[0], 1.f);
		m_ui.drawCircle({ 1043.60357597866f, 676.183738511636f }, { 7.5f }, speed < m_speedThresholds[0] ? m_speedUIColours[1] : noColor, 1.f);
		m_ui.drawCircle({ 1064.47285819127f, 675.142292174748f }, { 7.5f }, speed < m_speedThresholds[1] ? m_speedUIColours[2] : noColor, 1.f);
		m_ui.drawCircle({ 1085.34214040388f, 673.934247237929f }, { 7.5f }, speed < m_speedThresholds[2] ? m_speedUIColours[3] : noColor, 1.f);
		m_ui.drawCircle({ 1106.21142261648f, 672.557881377036f }, { 7.5f }, speed < m_speedThresholds[3] ? m_speedUIColours[4] : noColor, 1.f);
		m_ui.drawCircle({ 1127.08070482909f, 671.011203218924f }, { 7.5f }, speed < m_speedThresholds[4] ? m_speedUIColours[5] : noColor, 1.f);
		{
			ZoneScopedN("SeventhTopCircleDraw");
			m_ui.drawCircle({ 1147.9499870417f,  669.291937293714f }, { 7.5f }, speed < m_speedThresholds[5] ? m_speedUIColours[6] : noColor, 1.f);
		}
		m_ui.drawCircle({ 1168.81926925431f, 667.397506369484f }, { 7.5f }, speed < m_speedThresholds[6] ? m_speedUIColours[7] : noColor, 1.f);
		m_ui.drawCircle({ 1189.68855146691f, 665.325010832928f }, { 7.5f }, speed < m_speedThresholds[7] ? m_speedUIColours[8] : noColor, 1.f);
		m_ui.drawCircle({ 1210.55783367952f, 663.071204707666f }, { 7.5f }, speed < m_speedThresholds[8] ? m_speedUIColours[9] : noColor, 1.f);
		m_ui.drawCircle({ 1231.42711589213f, 660.632467814566f }, { 7.5f }, speed < m_speedThresholds[9] ? m_speedUIColours[10] : noColor, 1.f);
		m_ui.drawCircle({ 1252.29639810473f, 658.004773471693f }, { 7.5f }, speed < m_speedThresholds[10] ? m_speedUIColours[11] : noColor, 1.f);
		m_ui.drawCircle({ 1273.16568031734f, 655.183650999685f }, { 7.5f }, speed < m_speedThresholds[11] ? m_speedUIColours[12] : noColor, 1.f);
		m_ui.drawCircle({ 1294.03496252995f, 652.164142134382f }, { 7.5f }, speed < m_speedThresholds[12] ? m_speedUIColours[13] : noColor, 1.f);
		m_ui.drawCircle({ 1314.90424474256f, 648.940750242794f }, { 7.5f }, speed < m_speedThresholds[13] ? m_speedUIColours[14] : noColor, 1.f);
		m_ui.drawCircle({ 1335.77352695516f, 645.50738097831f }, { 7.5f }, speed < m_speedThresholds[14] ? m_speedUIColours[15] : noColor, 1.f);


		// Target circles
		glm::vec4 targetCircleColour(0.909803921568627f, 0.f, 0.f, 0.65f);
		{
			ZoneScopedN("TargetCircles");
			m_ui.drawCircle({ 184.f, 618.f }, { 50.f }, targetCircleColour, 0.075f);
			m_ui.drawCircle({ 184.f, 618.f }, { 75.f }, targetCircleColour, 0.0375f);
			m_ui.drawCircle({ 184.f, 618.f }, { 100.f }, targetCircleColour, 0.01875f);
			m_ui.drawCircle({ 184.f, 618.f }, { 125.f }, targetCircleColour, 0.009375f);
		}
		// Close Ship Circles 
		{
			ZoneScopedN("CloseShipCircles");
			for (auto& circle : m_closeAsteroids) {
				float radius = 7.5f + circle.z / 2.5f;
				m_ui.drawCircle({ 184.f + circle.x * 5.0f, 618.f + circle.y * 5.f }, { radius }, { targetCircleColour.x, targetCircleColour.y, targetCircleColour.z, circle.z / 50.f + 0.25f }, 0.2f);
			}
		}
		// Close Target quads
		{
			ZoneScopedN("CloseTargetQuads");
			for (auto& quad : m_closeTargets) {
				float size = 10.f + quad.z / 3.5f;
				m_ui.drawQuad({ 184.f + quad.x * 5.0f - size / 2.f, 618.f + quad.y * 5.f - size / 2.f }, { size, size }, { 0.463f, 0.031f, 0.769f, quad.z / 35.f });
			}
		}
	}

	m_ui.end();
	m_ui.onRender(); // Flush UI batches

	glDisable(GL_BLEND);
}

void AsteriodBelt::onUpdate(float timestep)
{
	ZoneScopedN("OnUpdate");
	if (m_state == GameState::running)
	{
		timestep = std::clamp(timestep, 0.f, 0.1f);

		auto view = m_mainScene->m_entities.view<ScriptComp>();
		for (auto entity : view)
		{

			auto& script = view.get<ScriptComp>(entity);
			script.onUpdate(timestep);

		}

		m_broadPhase.onUpdate(timestep); // Update broadphase based on the value of timestep

		//Check for and action collisions
		m_closeAsteroids.clear();
		m_closeTargets.clear();
		checkWaypointCollisions();
		checkAsteroidCollisions();

		//Using camera position and the position of each asteroid, I iterate through a for loop of entities,
		// which check the distance between the camera and each asteroid's position. It assigns a size_t
		// value to lodLevel from 0 to 2 and then uses the component script for lodassign to assign the index
		// which is used in the renderer class, to use the correct LOD data. For example, lodLevel being 2 means
		// it has the least indices and 0 has the most, but it is closer and can use more detail.
		auto& cameraTransform = m_mainScene->m_entities.get<Transform>(camera);
		
		glm::vec3 cameraPos = cameraTransform.translation;
		
		//For distance calculation for asteroid/LODIndex value changing.
		
		auto asteroidView = m_mainScene->m_entities.view<Transform, Render, LODAssign>();
		for (auto entity : asteroidView)
		{
			auto& asteroidTransform = m_mainScene->m_entities.get<Transform>(entity);
			glm::vec3 asteroidPos = asteroidTransform.translation;

			float distance = glm::distance(cameraPos, asteroidPos);

			if (distance <= 25.0f) lodLevel = 0;
			else if (distance <= 100.0f) lodLevel = 1;
			else lodLevel = 2;

			auto& lodComp = m_mainScene->m_entities.get<LODAssign>(entity);
			lodComp.lodIndex = lodLevel;

			
		}

		auto& pass = m_mainRenderer.getRenderPass(0);

		pass.camera.updateView(cameraTransform.transform);
		pass.UBOmanager.setCachedValue("b_camera", "u_view", pass.camera.view);
		pass.UBOmanager.setCachedValue("b_camera", "u_viewPos", cameraTransform.translation);

		auto& skyboxRenderComp = m_mainScene->m_entities.get<Render>(skyBox);
		skyboxRenderComp.material->setValue("u_skyboxView", glm::mat4(glm::mat3(pass.camera.view)));


		//Draw UI
		
		m_ui.begin();

	}
}

void AsteriodBelt::onImGUIRender()
{
	ZoneScopedN("onImGUIRender");
	// Scripts widgets
	if (ImGui::TreeNode("Script settings"))
	{
		auto view = m_mainScene->m_entities.view<ScriptComp>();
		for (auto entity : view)
		{
			view.get<ScriptComp>(entity).onImGuiRender();
		}
		ImGui::TreePop();
	}
	// Bloom detail
	//m_bloomPanel.onImGuiRender();

}

void AsteriodBelt::onKeyPressed(KeyPressedEvent& e)
{
	if (e.getKeyCode() == GLFW_KEY_SPACE && m_state == GameState::intro) m_state = GameState::running;
}

void AsteriodBelt::checkWaypointCollisions()
{
	ZoneScopedN("CheckWaypointCollision");
	TracyGpuZone("checkWaypointCollisions");
	// Generate collision point in front of ship
	auto& shipTransform = m_mainScene->m_entities.get<Transform>(ship);

	glm::vec3 shipRight = { -shipTransform.transform[0][0], -shipTransform.transform[0][1], -shipTransform.transform[0][2] };
	glm::vec3 shipUp = { -shipTransform.transform[1][0], -shipTransform.transform[1][1], -shipTransform.transform[1][2] };
	glm::vec3 shipForward = { -shipTransform.transform[2][0], -shipTransform.transform[2][1], -shipTransform.transform[2][2] };

	OBBCollider obb(glm::vec3(0.72f, 0.18f, 1.f), ship);

	auto view = m_mainScene->m_entities.view<SphereCollider, Transform>();


	for (auto& entity : view)
	{

		auto& sphereCollider = view.get<SphereCollider>(entity);
		float dist = Physics::DistanceOBBToSphere(m_mainScene.get(), obb, sphereCollider);
		if (dist < 25.f)
		{

			glm::vec3 shipToAsteroid = view.get<Transform>(entity).translation - m_mainScene->m_entities.get<Transform>(ship).translation;

			if (glm::dot(shipToAsteroid, shipForward) > 0)
			{

				float x = glm::dot(shipRight, shipToAsteroid);
				float y = glm::dot(shipUp, shipToAsteroid);
				m_closeAsteroids.push_back(glm::vec3(x, y, 25.f - dist));

			}

		}
		if (dist < 0.f) m_state = GameState::gameOver;
	}

	
}

void AsteriodBelt::checkAsteroidCollisions()
{

	//Set the off-set.
	glm::vec3 offset(0.f, 0.18f, -0.14f);

	//Assign transform functions to the ship, to calculate right, up and forward vectors.
	auto& shipTransform = m_mainScene->m_entities.get<Transform>(ship);

	glm::vec3 shipRight = { -shipTransform.transform[0][0], -shipTransform.transform[0][1], -shipTransform.transform[0][2] };
	glm::vec3 shipUp = { -shipTransform.transform[1][0], -shipTransform.transform[1][1], -shipTransform.transform[1][2] };
	glm::vec3 shipForward = { shipTransform.transform[2][0], shipTransform.transform[2][1], shipTransform.transform[2][2] };

	glm::vec3 hitPoint = shipTransform.translation;

	hitPoint += shipRight * offset.x;
	hitPoint += shipUp * offset.y;
	hitPoint += shipForward * offset.z;

	// Get all entities with a sphere collider
	auto view = m_mainScene->m_entities.view<OBBCollider, Transform, Order>();

	for (auto& entity : view) {
		// Compute distance
		auto& obbCollider = view.get<OBBCollider>(entity);
		float dist = Physics::DistanceOBBToPoint(m_mainScene.get(), obbCollider, hitPoint);

		if (dist < 0.75f) {
			// Hit

			// Make a record of order
			auto order = view.get<Order>(entity);

			//Create an entity called index and set it to null.
			entt::entity index{ entt::null };
			uint32_t lowest{ std::numeric_limits<uint32_t>::max() };

			auto orderView = m_mainScene->m_entities.view<Order>();
			for (auto& entity2 : orderView) {
				auto order2 = orderView.get<Order>(entity2);
				if (order2.order > order.order && order2.order < lowest) {
					index = entity2;
					lowest = order2.order;
				}
			}

			// Give material to next target
			if (index != entt::null) {
				// Set material
				auto& target = m_mainScene->m_entities.get<Render>(index);
				target.material = m_mainScene->m_entities.get<Render>(nextTarget).material;
				// Update next target
				nextTarget = index;
			}

			// Remove waypoint that has been hit
			m_mainScene->m_entities.destroy(entity);
		}

		if (dist < 35.f) {
			// Ship to asteroid
			glm::vec3 shipToTarget = view.get<Transform>(entity).translation - m_mainScene->m_entities.get<Transform>(ship).translation;
			// Project to 2d for UI if infront of ship
			if (glm::dot(shipToTarget, shipForward) > 0) {
				float x = glm::dot(shipRight, shipToTarget);
				float y = glm::dot(shipUp, shipToTarget);
				m_closeTargets.push_back(glm::vec3(x, y, 35.f - dist));

			}
		}
	}


}




void AsteriodBelt::generateLevel()
{
	ZoneScopedN("generateLevel");
	TracyGpuZone("generateLevel");

	const int wayPointCount = 100; // Number of waypoints
	const int asteroidsPerWayPointCount = 20; // Number of asteroids per way Point
	const float innerRadius = 20.f; // Closest the centre of an asteriod can be
	const float outerRadius = 80.f; // Furthest the centre of an asteriod can be

	m_mainScene.reset(new Scene);
	
	// Skybox
	ShaderDescription skyboxShaderDesc;
	skyboxShaderDesc.type = ShaderType::rasterization;
	skyboxShaderDesc.vertexSrcPath = "./assets/shaders/Skybox/Vert.glsl";
	skyboxShaderDesc.fragmentSrcPath = "./assets/shaders/Skybox/Frag.glsl";

	std::shared_ptr<Shader> skyboxShader;
	skyboxShader = std::make_shared<Shader>(skyboxShaderDesc);

	std::vector<float> skyboxVertices = {
		// positions          
		-1000.f,  1000.f, -1000.f,
		-1000.f, -1000.f, -1000.f,
		1000.f, -1000.f, -1000.f,
		1000.f, -1000.f, -1000.f,
		1000.f,  1000.f, -1000.f,
		-1000.f,  1000.f, -1000.f,

		-1000.f, -1000.f,  1000.f,
		-1000.f, -1000.f, -1000.f,
		-1000.f,  1000.f, -1000.f,
		-1000.f,  1000.f, -1000.f,
		-1000.f,  1000.f,  1000.f,
		-1000.f, -1000.f,  1000.f,

		1000.f, -1000.f, -1000.f,
		1000.f, -1000.f,  1000.f,
		1000.f,  1000.f,  1000.f,
		1000.f,  1000.f,  1000.f,
		1000.f,  1000.f, -1000.f,
		1000.f, -1000.f, -1000.f,

		-1000.f, -1000.f,  1000.f,
		-1000.f,  1000.f,  1000.f,
		1000.f,  1000.f,  1000.f,
		1000.f,  1000.f,  1000.f,
		1000.f, -1000.f,  1000.f,
		-1000.f, -1000.f,  1000.f,

		-1000.f,  1000.f, -1000.f,
		1000.f,  1000.f, -1000.f,
		1000.f,  1000.f,  1000.f,
		1000.f,  1000.f,  1000.f,
		-1000.f,  1000.f,  1000.f,
		-1000.f,  1000.f, -1000.f,

		-1000.f, -1000.f, -1000.f,
		-1000.f, -1000.f,  1000.f,
		1000.f, -1000.f, -1000.f,
		1000.f, -1000.f, -1000.f,
		-1000.f, -1000.f,  1000.f,
		1000.f, -1000.f,  1000.f
	};
	std::vector<uint32_t> skyboxIndices(skyboxVertices.size() / 3);
	std::iota(skyboxIndices.begin(), skyboxIndices.end(), 0);

	std::shared_ptr<VAO> skyboxVAO;
	skyboxVAO = std::make_shared<VAO>(skyboxIndices);
	skyboxVAO->addVertexBuffer(skyboxVertices, { {GL_FLOAT, 3} });

	std::array<const char*, 6> cubeMapPaths = {
		"./assets/textures/Skybox/right.png",
		"./assets/textures/Skybox/left.png",
		"./assets/textures/Skybox/top.png",
		"./assets/textures/Skybox/bottom.png",
		"./assets/textures/Skybox/front.png",
		"./assets/textures/Skybox/back.png"
	};

	std::shared_ptr<CubeMap> cubeMap;
	cubeMap = std::make_shared<CubeMap>(cubeMapPaths, false);

	std::shared_ptr<Material> skyboxMaterial;
	skyboxMaterial = std::make_shared<Material>(skyboxShader);
	skyboxMaterial->setValue("u_skybox", cubeMap);

	DirectionalLight dl;
	dl.direction = glm::normalize(glm::vec3(1.f, -3.5f, -2.f));
	m_mainScene->m_directionalLights.push_back(dl);

	{
		camera = m_mainScene->m_entities.create();
		m_mainScene->m_entities.emplace<Transform>(camera);
	}

	{
		skyBox = m_mainScene->m_entities.create();
		auto& renderComp = m_mainScene->m_entities.emplace<Render>(skyBox);
		renderComp.geometry = skyboxVAO;
		renderComp.material = skyboxMaterial;
		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(skyBox);
		skyboxMaterial->setValue("u_skyboxView", glm::inverse(transformComp.transform));

		auto& lodComp = m_mainScene->m_entities.emplace<LODAssign>(skyBox);
		lodComp.lodNumber = lodNonAsteroid;

	}


	ShaderDescription pbrShaderDesc;
	pbrShaderDesc.type = ShaderType::rasterization;
	pbrShaderDesc.vertexSrcPath = "./assets/shaders/PBR/pbrVertex.glsl";
	pbrShaderDesc.fragmentSrcPath = "./assets/shaders/PBR/pbrFrag.glsl";

	std::shared_ptr<Shader> pbrShader;
	pbrShader = std::make_shared<Shader>(pbrShaderDesc);

	ShaderDescription pbrEShaderDesc;
	pbrEShaderDesc.type = ShaderType::rasterization;
	pbrEShaderDesc.vertexSrcPath = "./assets/shaders/PBR/pbrVertex.glsl";
	pbrEShaderDesc.fragmentSrcPath = "./assets/shaders/PBR/pbrFragEmissive.glsl";

	std::shared_ptr<Shader> pbrEmissiveShader;
	pbrEmissiveShader = std::make_shared<Shader>(pbrEShaderDesc);

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

	//Ship
	Model shipModel("./assets/models/ship/Spaceship.obj", attributeTypes);
	// Body
	std::shared_ptr<VAO> shipVAO;
	shipVAO = std::make_shared<VAO>(shipModel.m_meshes[0].indices);
	shipVAO->addVertexBuffer(shipModel.m_meshes[0].vertices, modelLayout);

	std::shared_ptr<Texture> ship_albedo = std::make_shared<Texture>("./assets/models/ship/albedo.jpg");
	std::shared_ptr<Texture> ship_normal = std::make_shared<Texture>("./assets/models/ship/normal.jpg");
	std::shared_ptr<Texture> ship_metal = std::make_shared<Texture>("./assets/models/ship/metallic.jpg");
	std::shared_ptr<Texture> ship_rough = std::make_shared<Texture>("./assets/models/ship/roughness.jpg");
	std::shared_ptr<Texture> ship_AO = std::make_shared<Texture>("./assets/models/ship/AO.png");

	std::shared_ptr<Material> shipMaterial;
	shipMaterial = std::make_shared<Material>(pbrShader);
	shipMaterial->setValue("albedoTexture", ship_albedo);
	shipMaterial->setValue("normalTexture", ship_normal);
	shipMaterial->setValue("roughTexture", ship_rough);
	shipMaterial->setValue("metalTexture", ship_metal);
	shipMaterial->setValue("aoTexture", ship_AO);

	{
		ZoneScopedN("Ship");
		TracyGpuZone("Ship");
		ship = m_mainScene->m_entities.create();

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(ship);
		renderComp.geometry = shipVAO;
		renderComp.material = shipMaterial;

		auto& transformComp = m_mainScene->m_entities.emplace<Transform>(ship);
		transformComp.translation = glm::vec3(0.f, 0.f, -2.f);
		transformComp.rotation = glm::quat(glm::vec3(0.f, glm::pi<float>(), 0.f));
		transformComp.scale = glm::vec3(1.f);
		transformComp.recalc();

		auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(ship);

		auto& lodComp = m_mainScene->m_entities.emplace<LODAssign>(ship);
		lodComp.lodNumber = lodNonAsteroid;

		scriptComp.attachScript<ControllerScript>(ship, m_mainScene, m_winRef, camera, glm::vec3(0.06f, 0.06f, -1.5f), glm::vec3(0.f, 0.7f, 2.6f), &speed);
	}

	auto meshOpt = [this](Model model, VBOLayout vbo, std::shared_ptr<VAO>& allLODsVAO)
	{

		const size_t vertexCountOnLoad = model.m_meshes[0].vertices.size() / vertexComponents;
		const size_t indexCountOnLoad = model.m_meshes[0].indices.size();

		{

			std::vector<uint32_t> remap(model.m_meshes[0].vertices.size()); // Needs to take in vertices rather than indices, as we need vertex_count elements not indices.

			const size_t vertexCount = meshopt_generateVertexRemap(
				remap.data(),
				model.m_meshes[0].indices.data(),
				indexCountOnLoad,
				model.m_meshes[0].vertices.data(),
				vertexCountOnLoad,
				vertexStride
			);

			std::vector<uint32_t> remappedIndices(model.m_meshes[0].indices.size());
			std::vector<float> remappedVertices(vertexCount * vertexComponents);

			meshopt_remapIndexBuffer(
				remappedIndices.data(),
				model.m_meshes[0].indices.data(),
				indexCountOnLoad,
				remap.data()
			);

			meshopt_remapVertexBuffer(

				remappedVertices.data(),
				model.m_meshes[0].vertices.data(),
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

			std::vector<uint32_t> allIndices;
			allIndices.insert(allIndices.end(), m_indices.begin(), m_indices.end());
			allIndices.insert(allIndices.end(), m_indicesLOD1.begin(), m_indicesLOD1.end());
			allIndices.insert(allIndices.end(), m_indicesLOD2.begin(), m_indicesLOD2.end());

			allLODsVAO = std::make_shared<VAO>(allIndices);
			allLODsVAO->addVertexBuffer(m_vertices, vbo);

			allLODsVAO->LOD_data[0] = std::pair<uint32_t, uint32_t>(0, m_indices.size());
			allLODsVAO->LOD_data[1] = std::pair<uint32_t, uint32_t>(m_indices.size(), m_indicesLOD1.size());
			allLODsVAO->LOD_data[2] = std::pair<uint32_t, uint32_t>(m_indices.size() + m_indicesLOD1.size(), m_indicesLOD2.size());

			

		}

	};
	
	std::array<std::shared_ptr<VAO>, 4> asteroidVAOs;
	std::array<std::shared_ptr<Material>, 4> asteroidMaterials;
	{
		//	Asteroid 1
		Model asteroidModel("./assets/models/asteroid1/asteroid.obj", attributeTypes);
		//asteroidVAOs[0] = std::make_shared<VAO>(asteroidModel.m_meshes[0].indices);
		//asteroidVAOs[0]->addVertexBuffer(asteroidModel.m_meshes[0].vertices, modelLayout);

		std::shared_ptr<Texture> asteroid_albedo = std::make_shared<Texture>("./assets/models/asteroid1/albedo.jpg");
		std::shared_ptr<Texture> asteroid_normal = std::make_shared<Texture>("./assets/models/asteroid1/normal.jpg");
		std::shared_ptr<Texture> asteroid_metal = std::make_shared<Texture>("./assets/models/asteroid1/metallic.jpg");
		std::shared_ptr<Texture> asteroid_rough = std::make_shared<Texture>("./assets/models/asteroid1/roughness.jpg");
		std::shared_ptr<Texture> asteroid_AO = std::make_shared<Texture>("./assets/models/asteroid1/AO.png");


		asteroidMaterials[0] = std::make_shared<Material>(pbrShader);
		asteroidMaterials[0]->setValue("albedoTexture", asteroid_albedo);
		asteroidMaterials[0]->setValue("normalTexture", asteroid_normal);
		asteroidMaterials[0]->setValue("roughTexture", asteroid_rough);
		asteroidMaterials[0]->setValue("metalTexture", asteroid_metal);
		asteroidMaterials[0]->setValue("aoTexture", asteroid_AO);

		meshOpt(asteroidModel, modelLayout,asteroidVAOs[0]);
	}
	
	{
		//	Asteroid 2
		Model asteroidModel("./assets/models/asteroid2/asteroid.obj", attributeTypes);
		//asteroidVAOs[1] = std::make_shared<VAO>(asteroidModel.m_meshes[0].indices);
		//asteroidVAOs[1]->addVertexBuffer(asteroidModel.m_meshes[0].vertices, modelLayout);

		std::shared_ptr<Texture> asteroid_albedo = std::make_shared<Texture>("./assets/models/asteroid2/albedo.jpg");
		std::shared_ptr<Texture> asteroid_normal = std::make_shared<Texture>("./assets/models/asteroid2/normal.jpg");
		std::shared_ptr<Texture> asteroid_metal = std::make_shared<Texture>("./assets/models/asteroid2/metallic.jpg");
		std::shared_ptr<Texture> asteroid_rough = std::make_shared<Texture>("./assets/models/asteroid2/roughness.jpg");
		std::shared_ptr<Texture> asteroid_AO = std::make_shared<Texture>("./assets/models/asteroid2/AO.png");


		asteroidMaterials[1] = std::make_shared<Material>(pbrShader);
		asteroidMaterials[1]->setValue("albedoTexture", asteroid_albedo);
		asteroidMaterials[1]->setValue("normalTexture", asteroid_normal);
		asteroidMaterials[1]->setValue("roughTexture", asteroid_rough);
		asteroidMaterials[1]->setValue("metalTexture", asteroid_metal);
		asteroidMaterials[1]->setValue("aoTexture", asteroid_AO);

		meshOpt(asteroidModel, modelLayout, asteroidVAOs[1]);
	}
	
	{
		//	Asteroid 3
		Model asteroidModel("./assets/models/asteroid3/asteroid.obj", attributeTypes);
		asteroidVAOs[2] = std::make_shared<VAO>(asteroidModel.m_meshes[0].indices);
		asteroidVAOs[2]->addVertexBuffer(asteroidModel.m_meshes[0].vertices, modelLayout);

		std::shared_ptr<Texture> asteroid_albedo = std::make_shared<Texture>("./assets/models/asteroid3/albedo.jpg");
		std::shared_ptr<Texture> asteroid_normal = std::make_shared<Texture>("./assets/models/asteroid3/normal.jpg");
		std::shared_ptr<Texture> asteroid_metal = std::make_shared<Texture>("./assets/models/asteroid3/metallic.jpg");
		std::shared_ptr<Texture> asteroid_rough = std::make_shared<Texture>("./assets/models/asteroid3/roughness.jpg");
		std::shared_ptr<Texture> asteroid_AO = std::make_shared<Texture>("./assets/models/asteroid3/AO.png");


		asteroidMaterials[2] = std::make_shared<Material>(pbrShader);
		asteroidMaterials[2]->setValue("albedoTexture", asteroid_albedo);
		asteroidMaterials[2]->setValue("normalTexture", asteroid_normal);
		asteroidMaterials[2]->setValue("roughTexture", asteroid_rough);
		asteroidMaterials[2]->setValue("metalTexture", asteroid_metal);
		asteroidMaterials[2]->setValue("aoTexture", asteroid_AO);

		meshOpt(asteroidModel, modelLayout, asteroidVAOs[2]);
	}
	
	{
		//	Asteroid 4
		Model asteroidModel("./assets/models/asteroid4/asteroid.obj", attributeTypes);
		asteroidVAOs[3] = std::make_shared<VAO>(asteroidModel.m_meshes[0].indices);
		asteroidVAOs[3]->addVertexBuffer(asteroidModel.m_meshes[0].vertices, modelLayout);

		std::shared_ptr<Texture> asteroid_albedo = std::make_shared<Texture>("./assets/models/asteroid4/albedo.jpg");
		std::shared_ptr<Texture> asteroid_normal = std::make_shared<Texture>("./assets/models/asteroid4/normal.jpg");
		std::shared_ptr<Texture> asteroid_metal = std::make_shared<Texture>("./assets/models/asteroid4/metallic.jpg");
		std::shared_ptr<Texture> asteroid_rough = std::make_shared<Texture>("./assets/models/asteroid4/roughness.jpg");
		std::shared_ptr<Texture> asteroid_AO = std::make_shared<Texture>("./assets/models/asteroid4/AO.png");


		asteroidMaterials[3] = std::make_shared<Material>(pbrShader);
		asteroidMaterials[3]->setValue("albedoTexture", asteroid_albedo);
		asteroidMaterials[3]->setValue("normalTexture", asteroid_normal);
		asteroidMaterials[3]->setValue("roughTexture", asteroid_rough);
		asteroidMaterials[3]->setValue("metalTexture", asteroid_metal);
		asteroidMaterials[3]->setValue("aoTexture", asteroid_AO);

		meshOpt(asteroidModel, modelLayout, asteroidVAOs[3]);
	}
	
	// Waypoints
	ShaderDescription phongEmissiveShdrDesc;
	phongEmissiveShdrDesc.type = ShaderType::rasterization;
	phongEmissiveShdrDesc.vertexSrcPath = "./assets/shaders/Phong/Vert.glsl";
	phongEmissiveShdrDesc.fragmentSrcPath = "./assets/shaders/Phong/EmissiveFrag.glsl";

	std::shared_ptr<Shader> phongEmissiveShader;
	phongEmissiveShader = std::make_shared<Shader>(phongEmissiveShdrDesc);

	VBOLayout cubeLayout = {
		{GL_FLOAT, 3},
		{GL_FLOAT, 3},
		{GL_FLOAT, 2}
	};

	Model cubeModel("./assets/models/whiteCube/letterCube.obj");

	std::shared_ptr<VAO> cubeVAO;
	cubeVAO = std::make_shared<VAO>(cubeModel.m_meshes[0].indices);
	cubeVAO->addVertexBuffer(cubeModel.m_meshes[0].vertices, cubeLayout);

	std::shared_ptr<Texture> cubeTexture;
	cubeTexture = std::make_shared<Texture>("./assets/models/whiteCube/letterCube.png");

	std::shared_ptr<Material> cubeMaterial;
	cubeMaterial = std::make_shared<Material>(phongEmissiveShader);
	cubeMaterial->setValue("u_albedo", glm::vec3(0.722f, 0.251f, 0.871f));
	cubeMaterial->setValue("u_emissive", glm::vec4(0.722f, 0.251f, 0.871f, 4.75f));
	cubeMaterial->setValue("u_albedoMap", cubeTexture);

	std::shared_ptr<Material> firstCubeMaterial;
	firstCubeMaterial = std::make_shared<Material>(phongEmissiveShader);
	firstCubeMaterial->setValue("u_albedo", glm::vec3(0.392f, 0.859f, 0.196f));
	firstCubeMaterial->setValue("u_emissive", glm::vec4(0.392f, 0.859f, 0.196f, 4.75f));
	firstCubeMaterial->setValue("u_albedoMap", cubeTexture);

	//Create the cube entity for the waypoints.
	entt::entity cube = m_mainScene->m_entities.create();
	m_mainScene->m_entities.emplace<Transform>(cube);

	for (int i = 0; i < wayPointCount - 1; i++)
	{
		ZoneScopedN("Waypoints");
		TracyGpuZone("Waypoints");
		auto& transformComp = m_mainScene->m_entities.get<Transform>(cube);
		glm::vec3 right = { -transformComp.transform[0][0], -transformComp.transform[0][1], -transformComp.transform[0][2] };
		glm::vec3 up = { -transformComp.transform[1][0], -transformComp.transform[1][1], -transformComp.transform[1][2] };
		glm::vec3 forward = { -transformComp.transform[2][0], -transformComp.transform[2][1], -transformComp.transform[2][2] };

		entt::entity oldCube = cube;

		cube = m_mainScene->m_entities.create();
		if (i == 0) nextTarget = cube;

		auto& renderComp = m_mainScene->m_entities.emplace<Render>(cube);
		renderComp.geometry = cubeVAO;
		if (i == 0) renderComp.material = firstCubeMaterial;
		else renderComp.material = cubeMaterial;

		auto& newTransformComp = m_mainScene->m_entities.emplace<Transform>(cube);
		newTransformComp.scale = glm::vec3(0.5f);

		auto& lodComp = m_mainScene->m_entities.emplace<LODAssign>(cube);
		lodComp.lodNumber = lodNonAsteroid;

		auto& order = m_mainScene->m_entities.emplace<Order>(cube);
		order.order = i;

		float fwdDelta = Randomiser::uniformFloatBetween(25.f, 35.f);
		newTransformComp.translation = transformComp.translation + forward * fwdDelta;

		glm::vec3 EulerAngle(Randomiser::uniformFloatBetween(-0.3f, 0.3f), Randomiser::uniformFloatBetween(-0.4f, 0.4f), 0.f);
		glm::quat angleDelta(EulerAngle);
		newTransformComp.rotation *= angleDelta;
		newTransformComp.recalc();
		m_mainScene->m_entities.emplace<OBBCollider>(cube, newTransformComp.scale * 0.5f, cube);

		//Asteroids
		std::vector<Transform> asteroidsThisWaypoint;
		asteroidsThisWaypoint.reserve(asteroidsPerWayPointCount);

		for (int j = 0; j < asteroidsPerWayPointCount; j++)
		{

			

			float t{ 0.f }, radius{ 0.f }, theta{ 0.f }, scale{ 0.f };
			glm::vec3 position(0.f);
			const float distThresholdSquared{ 1.f };

			bool tooClose{ true };
			while (tooClose)
			{

				t = Randomiser::uniformFloatBetween(0.f, 1.f);

				radius = Randomiser::uniformFloatBetween(innerRadius, outerRadius);

				theta = Randomiser::uniformFloatBetween(-glm::pi<float>(), glm::pi<float>());

				scale = Randomiser::uniformFloatBetween(0.4f, 5.f);


				position = m_mainScene->m_entities.get<Transform>(oldCube).translation;
				position += (forward * fwdDelta) * t;
				position.x += cos(theta) * radius;
				position.y += sin(theta) * radius;

				tooClose = false;
				for (auto& other : asteroidsThisWaypoint)
				{

					float dist = glm::length(position - other.translation);
					if (dist < distThresholdSquared)
					{

						tooClose = true;
						break;

					}

				}
			}



			ZoneScopedN("Asteroids");
			TracyGpuZone("Asteroids");

			asteroid = m_mainScene->m_entities.create();

			auto& renderComp = m_mainScene->m_entities.emplace<Render>(asteroid);
			auto modelIdx = Randomiser::uniformIntBetween(0, 3);
			renderComp.geometry = asteroidVAOs[modelIdx];
			renderComp.material = asteroidMaterials[modelIdx];

			auto& transformComp = m_mainScene->m_entities.emplace<Transform>(asteroid);
			transformComp.translation = position;
			transformComp.scale = glm::vec3(scale, scale, scale);
			transformComp.recalc();

			auto& lodComp = m_mainScene->m_entities.emplace<LODAssign>(asteroid);
			lodComp.lodNumber = lodAsteroid;
			lodComp.lodIndex = lodLevel;

			//Attach rotation script to the asteroids so that they rotate in the scene.
			auto x = Randomiser::uniformFloatBetween(-1.f, 1.f);
			auto y = Randomiser::uniformFloatBetween(-1.f, 1.f);
			auto z = Randomiser::uniformFloatBetween(-1.f, 1.f);
			auto& scriptComp = m_mainScene->m_entities.emplace<ScriptComp>(asteroid);
			scriptComp.attachScript<RotationScript>(asteroid, m_mainScene, glm::vec3(x, y, z), 0);
			m_mainScene->m_entities.emplace<SphereCollider>(asteroid, scale, asteroid);

			


		}

	}

	

	
}



