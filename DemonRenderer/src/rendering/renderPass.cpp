#include "rendering/renderPass.hpp"
#include "components/render.hpp"
#include <entt/entt.hpp>

void RenderPass::parseScene()
{

	auto renderView = scene->m_entities.view<Render>(); //!< Get all entities with a render component


	for (auto entity : renderView)
	{

		auto& renderComp = renderView.get<Render>(entity);

		if (renderComp.material)
		{

			for (auto& UBOlayout : renderComp.material->m_shader->m_UBOLayouts)
			{

				UBOmanager.addUBO(UBOlayout);

			}

		}

	}


}
