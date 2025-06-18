#pragma once
#include "DemonRenderer.hpp"

class BloomPanel
{
public:
	BloomPanel(Renderer& renderer) : m_renderer(renderer) {}
	void onImGuiRender();
	float getThreshold() const { return bloomThreshold; }
private:
	Renderer& m_renderer;
	float bloomThreshold{ 1.5f };
};