/** \file lodassign.hpp*/
#pragma once
#include <entt/entt.hpp>
#include <memory>

/** \struct LODAssign
*   \brief Number assigned to renderable entities to determine if they should use LOD data or not.
*/

struct LODAssign
{
public:
	
	size_t lodIndex{ 0 };
	int lodNumber{ 0 };
};