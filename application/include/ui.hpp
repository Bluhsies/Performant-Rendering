#pragma once
// UI rendering, use Demon Render but also use programmable vertex pulling
#include "DemonRenderer.hpp"

struct QuadVertex {
	glm::vec2 position;
	glm::vec2 texCoords;
	glm::vec4 colour;
	float texSlot;
};

struct CircleVertex {
	glm::vec2 position;
	glm::vec2 unitPosition;
	glm::vec4 colour;
	float thickness;

};
class UI
{
public:
	void init(const glm::ivec2& size);
	void onRender() const;
	void begin(); // Clear all quads, circles and rounded quads
	void end(); // Render all quads, circles and rounded quads drawn since begin
	void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& colour); // Draw a quad at position (top-left) with size (width and height)
	void drawTexturedQuad(const glm::vec2& position, const glm::vec2& size, std::shared_ptr<Texture> texture); // Draw a quad at position (top-left) with size (width and height) and texture
	void drawCircle(const glm::vec2& centre, float radius, const glm::vec4& colour, float thickness); // Draw a circle at centre with radius;
private:
	static const int m_maxQuadCount = 256; // Maximum number of quads draws in the UI widget
	std::array<QuadVertex, m_maxQuadCount * 4> m_quadVertices; // Quad vertices, CPU side
	static const int m_maxCircleCount = 256; // Maximum number of circles draws in the UI widget
	std::array<CircleVertex, m_maxCircleCount * 4> m_circleVertices; // Circle vertices, CPU side
	std::array<int, 32> slots = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31 };
	Renderer m_UIRenderer; // Renderer to draw the scene
	std::shared_ptr<Texture> m_defaultTexture; // Used for drawing an untextured quad
	const glm::vec4 m_defaultColour{ glm::vec4(1.f) }; //Used for drawing a textured quad
	std::shared_ptr<Scene> m_UIScene; //Scene

	std::shared_ptr<SSBO> m_quadsSSBO; // SSBO to hold quad vertex data
	std::shared_ptr<SSBO> m_circlesSSBO; // SSBO to hold circle vertex data

	size_t m_currentQuadCount{ 0 }; // Number of quads currently set to be rendered
	glm::ivec2 m_size{ 0,0 }; // size of the UI	
	size_t m_currentCircleCount{ 0 }; // Number of circles currently set to be rendered
	entt::entity m_circles{ entt::null }; //Entity used instead of actor and set to null
	entt::entity m_quads{ entt::null }; //Entity used instead of actor and set to null
	int lodNonAsteroid = 2;
};
