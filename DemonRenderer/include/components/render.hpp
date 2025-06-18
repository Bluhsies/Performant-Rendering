/** \file render.hpp */
#pragma once

#include <memory>
#include "buffers/VAO.hpp"
#include "rendering/material.hpp"

/*

 struct Render
 brief A render component

 The simplest Renderable thing will have some geometry and a material.
 Depth versions of these can be used which only give vertex positions and only
 render depth.
 All geometry will take the form of a VAO, SSBO are to be rendered by programmable
 vertex pulling.

*/

struct Render
{

	std::shared_ptr<Material>  material{ nullptr };
	std::shared_ptr<VAO> geometry{ nullptr };
	std::shared_ptr<Material> depthMaterial{ nullptr };
	std::shared_ptr<VAO> depthGeometry{ nullptr };



};
