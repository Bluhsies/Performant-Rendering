#pragma once

#include "buffers/FBO.hpp"
#include "rendering/scene.hpp"
#include "buffers/UBOmanager.hpp"
#include "rendering/camera.hpp"

/** \struct ViewPort
*	\brief Portion of the target taken up by a render pass. This now includes overridden operators for == and !=
*    when comparing the ViewPort, as opposed to creating a new viewport class in the renderer.    
*/
struct ViewPort {
	int32_t x{ 0 };
	int32_t y{ 0 };
	uint32_t width{ 0 };
	uint32_t height{ 0 };

	bool operator==(const ViewPort& other) const {

		return x == other.x &&
			y == other.y &&
			width == other.width &&
			height == other.height;

	}

	bool operator!=(const ViewPort& other) const {

		return !(*this == other);

	}
};

/**	\struct DepthPass
*	\brief A render pass which only performs depth calculations
*/
struct DepthPass
{
	UBOManager UBOmanager; //!< Holds all UBOs required for this pass
	Camera camera; //!< Camera for the pass
	std::shared_ptr<FBO> target; //!< Render target
	std::shared_ptr<Scene> scene; //!< Scene being rendered
	ViewPort viewPort; //!< Portion of the render target being rendered too
	bool clearDepth{ true }; //!< Should the depth buffer be cleared by this parse?

	void parseScene(); //!< Populate variable based on the scene
};
