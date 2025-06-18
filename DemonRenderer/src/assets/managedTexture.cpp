#include <glad/gl.h>
#include "assets/managedTexture.hpp"

uint32_t ManagedTexture::s_ID = 0;

uint32_t ManagedTexture::getUnit()
{
	uint32_t unit = -1;
	bool needsBinding = ManagedTexture::m_textureUnitManager.getUnit(m_ID, unit);
	if (needsBinding) {
		if (s_ID != m_ID)
		{
			if (unit == -1) {
				ManagedTexture::m_textureUnitManager.clear();
				ManagedTexture::m_textureUnitManager.getUnit(m_ID, unit);
			}


			glBindTextureUnit(unit, m_ID);
			s_ID = m_ID;
		}
	}
	
	return unit;
}

uint32_t ManagedTexture::getUnit(bool& flushNotification, bool flushIfRequired)
{
	flushNotification = false;
	uint32_t unit = -1;
	bool needsBinding = ManagedTexture::m_textureUnitManager.getUnit(m_ID, unit);
	if (needsBinding) {
		if (unit == -1) {
			flushNotification = true;
			if (flushIfRequired) {
				ManagedTexture::m_textureUnitManager.clear();
				ManagedTexture::m_textureUnitManager.getUnit(m_ID, unit);
			}
			
		}
		else {
			if (s_ID != m_ID)
			{
				glBindTextureUnit(unit, m_ID);
				s_ID = m_ID;
			}
		}
	}
	return unit;
}