#pragma once
#include <DemonRenderer.hpp>
#include <entt/entt.hpp>

class LOD : public Layer
{

public:
	LOD(GLFWWindowImpl& win);
	void onRender() override;
	void onUpdate(float timestep) override;
	void onKeyPressed(KeyPressedEvent& e) override;
	void onImGUIRender() override;

private:
	RenderPass mainPass;
	Renderer m_mainRenderer;
	std::vector<uint32_t> m_indices;
	std::vector<float> m_vertices;
	std::vector<uint32_t> m_indicesLOD1;
	std::vector<uint32_t> m_indicesLOD2;
	std::vector<uint32_t> m_indicesLOD3;
	std::array<std::pair<uint32_t, uint32_t>, 3> LOD_data;
	std::shared_ptr<Scene> m_mainScene;
	const size_t vertexComponents = (3 + 3 + 2 + 3);
	const size_t vertexStride = sizeof(float) * vertexComponents;
	size_t LODindex{ 0 };
	entt::entity allLODs{ entt::null };
	bool m_wireframe{ false };


};
