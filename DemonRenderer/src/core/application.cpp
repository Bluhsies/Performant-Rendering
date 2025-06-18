/** \file application.cpp */

#include "core/application.hpp"
#include "tracy/Tracy.hpp"

Application::Application(const WindowProperties& winProps)
{
	m_window.open(winProps);

	m_window.handler.onWinClose = [this](WindowCloseEvent& e) {onClose(e);};
	m_window.handler.onKeyPress = [this](KeyPressedEvent& e) {onKeyPressed(e);};
	m_window.handler.onKeyRelease = [this](KeyReleasedEvent& e) {onKeyReleased(e);};

	auto setupTime = m_timer.reset();
}

void Application::run() 
{
    spdlog::debug("Application running");

	glEnable(GL_DEPTH_TEST);

	while (m_running) {	
		FrameMark;
		ZoneScopedN("Run");
		auto timestep = m_timer.reset();

		onUpdate(timestep);
		if(m_window.isHostingImGui()) onImGuiRender();
		onRender();

		m_window.onUpdate(timestep);
	}
}

void Application::onUpdate(float timestep)
{
	if (m_layer) m_layer->onUpdate(timestep);
}

void Application::onRender() const
{
	if (m_layer) m_layer->onRender();
}

void Application::onImGuiRender()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (m_ImGuiOpen)
	{
		ImGui::Begin("ImGUI Window", &m_ImGuiOpen, ImGuiWindowFlags_MenuBar);
		if(!m_ImGuiOpen) spdlog::info("ImGui window closed. Press I to reopen it.");
		
		if (m_layer) m_layer->onImGUIRender();

		ImGui::End();
	}

	ImGui::Render();
}

void Application::onClose(WindowCloseEvent& e)
{
	m_running = false;
	if (m_layer) m_layer->onClose(e);
	e.handle();
}

void Application::onResize(WindowResizeEvent& e)
{
	if (m_layer) m_layer->onResize(e);
}

void Application::onKeyPressed(KeyPressedEvent& e)
{
	if (e.getKeyCode() == GLFW_KEY_ESCAPE) {
		spdlog::info("Exiting");
		m_running = false;
		e.handle();
		return;
	}

	else{
		if (e.getKeyCode() == GLFW_KEY_I) m_ImGuiOpen = true;
		if (m_layer) m_layer->onKeyPressed(e);
		e.handle();
	}

}

void Application::onKeyReleased(KeyReleasedEvent& e)
{
	if (m_layer) m_layer->onKeyReleased(e);
}

void Application::onMousePress(MouseButtonPressedEvent& e)
{
	if (m_layer) m_layer->onMousePress(e);
}

void Application::onMouseReleased(MouseButtonReleasedEvent& e)
{
	if (m_layer) m_layer->onMouseReleased(e);
}

void Application::onMouseMoved(MouseMovedEvent& e)
{
	if (m_layer) m_layer->onMouseMoved(e);
}

void Application::onMouseScrolled(MouseScrolledEvent& e)
{
	if (m_layer) m_layer->onMouseScrolled(e);
}

void* operator new (std::size_t count)
{

	auto ptr = malloc(count);
	TracyAlloc(ptr, count);
	return ptr;

}

void operator delete (void* ptr) noexcept
{

	TracyFree(ptr);
	free(ptr);

}

//void* operator new[](std::size_t count)
//{
//
//	auto ptr = malloc(count);
//	TracyAlloc(ptr, count);
//	return ptr;
//
//}

//void operator delete[](void* ptr) noexcept
//{
//
//	TracyFree(ptr);
//	free(ptr);
//
//}