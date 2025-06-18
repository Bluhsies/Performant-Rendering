#include "buffers/SSBO.hpp"

std::unordered_map<uint32_t, uint32_t> SSBO::s_ID;

SSBO::SSBO(uint32_t size, uint32_t elementCount) : m_size(size), m_elementCount(elementCount)
{
   glCreateBuffers(1, &m_ID);
   glNamedBufferData(m_ID, m_size, nullptr, GL_DYNAMIC_DRAW);
}

SSBO::~SSBO()
{
	glDeleteBuffers(1, &m_ID);
}

void SSBO::bind(uint32_t unit)
{

	//This implementation seems to remove 2 API calls. I will check in the Uni lab tomorrow.
	/*
	*	Use of an unordered map to compare the value used in unit and ID, to a static variable called s_ID.
	*/

	auto ssboCheck = s_ID.find(unit);

	if (ssboCheck == s_ID.end() || ssboCheck->second != m_ID)
	{

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, unit, m_ID);
		s_ID[unit] = m_ID;

	}
	
	
}


