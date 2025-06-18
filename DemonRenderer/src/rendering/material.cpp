#include "rendering/material.hpp"
#include "tracy/TracyOpenGL.hpp"

uint32_t Material::s_ID = 0;

Material::Material(std::shared_ptr<Shader> shader, const std::string& transformUniformName) : 
	m_shader(shader),
	m_transformUniformName(transformUniformName)
{
	auto& infoCache = shader->m_uniformInfoCache;

	for (auto it = infoCache.begin(); it != infoCache.end(); ++it)
	{
		auto& name = it->first;
		auto& info = it->second;

		dataCache[name] = UniformData(info.type, info.size);

	}

	

}

void Material::unsetValue(const std::string& name)
{
	auto it = dataCache.find(name);
	if (it != dataCache.end()) {
		auto& data = dataCache[name];
		data.enabled = false;
	}
	else { spdlog::error("Could not set material value: {}. Not found in shader data cache.", name); }	
}

void Material::apply()
{
	ZoneScopedN("Material");
	TracyGpuZone("Material");
	// Bind shader
	// Checks to see if the id of m_shader is equal to the static variable s_ID.
	// If it isn't, it will call UseProgram and set s_ID to the value of m_shader->getID().
	if (s_ID != m_shader->getID())
	{

		glUseProgram(m_shader->getID());
		s_ID = m_shader->getID();

	}

	// Upload uniforms
	for (auto& dataPair : dataCache)
	{
		auto& name = dataPair.first;
		auto& matInfo = dataPair.second;

		if (matInfo.enabled)
		{
			auto& dispatchFunc = UniformConsts::UDT.at(matInfo.type);

			dispatchFunc(m_shader, name, matInfo);
			
			/*
			switch (matInfo.type)
			{
			case GL_FLOAT_VEC3:
				m_shader->uploadUniform(name, std::get<glm::vec3>(matInfo.data));
				break;
			case GL_SAMPLER_2D:
				m_shader->uploadUniform(name, std::get<std::shared_ptr<Texture>>(matInfo.data)->getUnit());
				break;
			}*/
		}
	}
}


