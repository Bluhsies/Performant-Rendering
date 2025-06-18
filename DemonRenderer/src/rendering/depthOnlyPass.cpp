#include "rendering/depthOnlyPass.hpp"
#include "components/render.hpp"
#include <entt/entt.hpp>

void DepthPass::parseScene()
{

	auto depthRenderView = scene->m_entities.view<Render>();

	for (auto entity : depthRenderView)
	{

		auto& renderComp = depthRenderView.get<Render>(entity);

		if (renderComp.depthMaterial)
		{

			for (auto& UBOlayout : renderComp.depthMaterial->m_shader->m_UBOLayouts)
			{

				UBOmanager.addUBO(UBOlayout);

			}

		}

	}

}


