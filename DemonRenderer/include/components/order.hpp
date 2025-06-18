#pragma once
/** \file order.hpp */
#include <limits>
#include <stdint.h>
#include <entt/entt.hpp>
#include <memory>

/** \struct Order
*   \brief Very simple component to allow gameobjects to have an order
*/

struct Order
{
public:
	uint32_t order{ std::numeric_limits<uint32_t>::max() };
};