#pragma once

#include "rendering/renderer.hpp"
#include "tracy/TracyOpenGL.hpp"
#include <entt/entt.hpp>
#include "components/render.hpp"
#include "components/transform.hpp"
#include "components/lodassign.hpp"
#include <iostream>

uint32_t Renderer::s_ID = 0;
ViewPort Renderer::s_viewID = {-1,-1,-1,-1}; //Use arbitrary values for the static variable to check the viewport transforms against, which are then overwritten to make
//sure it doesn't try to set the viewport more than once.

void Renderer::addRenderPass(const RenderPass& pass)
{
	m_renderOrder.push_back(std::pair<PassType, size_t>(PassType::render, m_renderPasses.size()));
	m_renderPasses.push_back(pass);
}

void Renderer::addDepthPass(const DepthPass& depthPass)
{
	m_renderOrder.push_back(std::pair<PassType, size_t>(PassType::depth, m_depthPasses.size()));
	m_depthPasses.push_back(depthPass);
}

void Renderer::addComputePass(const ComputePass& pass)
{
	m_renderOrder.push_back(std::pair<PassType, size_t>(PassType::compute, m_computePasses.size()));
	m_computePasses.push_back(pass);
}


RenderPass& Renderer::getRenderPass(size_t index)
{
	auto& [passType, passIdx] = m_renderOrder[index];
	return m_renderPasses[passIdx];
}

DepthPass& Renderer::getDepthPass(size_t index)
{
	auto& [passType, passIdx] = m_renderOrder[index];
	return m_depthPasses[passIdx];
}

ComputePass& Renderer::getComputePass(size_t index)
{
	auto& [passType, passIdx] = m_renderOrder[index];
	return m_computePasses[passIdx];
}

void Renderer::render() const
{
	ZoneScopedN("OverallRPass");
	TracyGpuZone("OverallRPass");

	auto& mainPass = m_renderPasses[0];
	CameraFrustrum cameraFrustum(mainPass.camera);

	for (auto& [passType, idx] : m_renderOrder)
	{
		if (passType == PassType::render)
		{
			ZoneScopedN("RPass");
			TracyGpuZone("RPass");
			auto& renderPass = m_renderPasses[idx];

			if (s_ID != renderPass.target->getID())
			{
				renderPass.target->use();
				s_ID = renderPass.target->getID();
			}
			setViewport(renderPass.viewPort.x, renderPass.viewPort.y, renderPass.viewPort.width, renderPass.viewPort.height);

			if (renderPass.clearDepth && renderPass.clearColour) {
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			}
			else if (renderPass.clearDepth) {
				glClear(GL_DEPTH_BUFFER_BIT);
			}
			else if (renderPass.clearColour) {
				glClear(GL_COLOR_BUFFER_BIT);
			}

			renderPass.UBOmanager.uploadCachedValues();

			// Get all entities with render and transform component
			auto view = renderPass.scene->m_entities.view<Render, Transform, LODAssign>();

			view.each([this, &cameraFrustum, &renderPass](entt::entity entity, const auto& renderComp, const auto& transformComp, const auto& lodComp) 
			{

					AABB aabb;
					bool hasAABB = renderPass.scene->m_entities.all_of<AABB>(entity);

					if (hasAABB)
					{

						aabb = renderPass.scene->m_entities.get<AABB>(entity);

					}

					//Perform frustum culling if an AABB exists
					if (hasAABB && !cameraFrustum.intersects(aabb)) {
						return;
					}


				ZoneScopedN("Entity");
				TracyGpuZone("Entity");
				if (renderComp.material)
				{
					ZoneScopedN("Material");
					TracyGpuZone("Material");
					renderComp.material->apply();
					if (renderComp.material->getTransformUniformName().length() > 0)
					{
						renderComp.material->m_shader->uploadUniform(renderComp.material->getTransformUniformName(), transformComp.transform);
					}

					if (renderComp.geometry)
					{
						ZoneScopedN("Draw");
						TracyGpuZone("Draw");

						

						//Added if statement to prevent redundant binding calls as it calls getID when binding the vertex array.
						if (s_ID != renderComp.geometry->getID())
						{

							glBindVertexArray(renderComp.geometry->getID());
							
							if (lodComp.lodNumber == 1)
							{
															

								void* baseVertexIndex = (void*)(sizeof(GLuint) * renderComp.geometry->LOD_data[lodComp.lodIndex].first);
								auto& drawCount = renderComp.geometry->LOD_data[lodComp.lodIndex].second;
								glDrawElements(renderComp.material->getPrimitive(), drawCount, GL_UNSIGNED_INT, baseVertexIndex);
								s_ID = renderComp.geometry->getID();

							}
							else
							{
																
								glDrawElements(renderComp.material->getPrimitive(), renderComp.geometry->getDrawCount(), GL_UNSIGNED_INT, NULL);
								s_ID = renderComp.geometry->getID();

							}

							
						}
					}
				}

			});

		}
		

		else if (passType == PassType::depth)
		{
			glCullFace(GL_FRONT);
			auto& depthPass = m_depthPasses[idx];

			if (s_ID != depthPass.target->getID())
			{
				depthPass.target->use();
				s_ID = depthPass.target->getID();
			}

			setViewport(depthPass.viewPort.x, depthPass.viewPort.y, depthPass.viewPort.width, depthPass.viewPort.height);

			if (depthPass.clearDepth) {
				glClear(GL_DEPTH_BUFFER_BIT);
			}

			depthPass.UBOmanager.uploadCachedValues();

			auto depthView = depthPass.scene->m_entities.view<Render, Transform>();

			depthView.each([](const auto& renderComp, const auto& transformComp)
				{

					ZoneScopedN("Entity");
					TracyGpuZone("Entity");
					if (renderComp.depthMaterial)
					{
						ZoneScopedN("Material");
						TracyGpuZone("Material");
						renderComp.depthMaterial->apply();
						if (renderComp.depthMaterial->getTransformUniformName().length() > 0)
						{
							renderComp.depthMaterial->m_shader->uploadUniform(renderComp.depthMaterial->getTransformUniformName(), transformComp.transform);
						}

						if (renderComp.depthGeometry)
						{
							ZoneScopedN("Draw");
							TracyGpuZone("Draw");
							//Added if statement to prevent redundant binding calls as it calls getID when binding the vertex array.
							if (s_ID != renderComp.depthGeometry->getID())
							{
								glBindVertexArray(renderComp.depthGeometry->getID());
								glDrawElements(renderComp.depthMaterial->getPrimitive(), renderComp.depthGeometry->getDrawCount(), GL_UNSIGNED_INT, NULL);
								s_ID = renderComp.depthGeometry->getID();
							}
						}
					}

				});

			

			glCullFace(GL_BACK);
		}
		else if (passType == PassType::compute)
		{
			auto& computePass = m_computePasses[idx];

			// Bind images -- Note: Could consider an image unit manager if this becomes a hot path
			for (auto& img : computePass.images) {
				GLboolean layered = img.layer ? GL_TRUE : GL_FALSE;

				GLenum access = 0;
				switch (img.access)
				{
				case TextureAccess::ReadOnly:
					access = GL_READ_ONLY;
					break;
				case TextureAccess::WriteOnly:
					access = GL_WRITE_ONLY;
					break;
				case TextureAccess::ReadWrite:
					access = GL_READ_WRITE;
					break;
				}

				GLenum fmt = 0; // Ignoring depth for now
				if (img.texture->isHDR()) {
					if (img.texture->getChannels() == 3) fmt = GL_RGB16F;
					else fmt = GL_RGBA16F;
				}
				else {
					if (img.texture->getChannels() == 3) fmt = GL_RGB8;
					else fmt = GL_RGBA8;
				}

				// Need to deal with layers for cubemap
				//Added if statement for preventing redundant bind calls, as it calls getID when binding the image texture.
				if (s_ID != img.texture->getID())
				{
					glBindImageTexture(img.imageUnit, img.texture->getID(), img.mipLevel, layered, 0, access, fmt);
					s_ID = img.texture->getID();
				}
			}

			computePass.material->apply();
			auto& wg = computePass.workgroups;
			glDispatchCompute(wg.x, wg.y, wg.z);
			if (computePass.barrier != MemoryBarrier::None) glMemoryBarrier(static_cast<GLbitfield>(computePass.barrier));
		}
	}

}

//Checks to see if the viewport is the same as s_viewID. If not, it will set the viewport and change s_viewID to be
//equal to newViewport. This is then called in render for the render and depth passes.
//Check seems to reduce the number of API calls in renderdoc by 4.
void Renderer::setViewport(int x, int y, int width, int height) const
{

	ViewPort newViewport = { x, y, width, height };

	if (newViewport != s_viewID)
	{

		glViewport(x, y, width, height);
		s_viewID = newViewport;


	}

}



