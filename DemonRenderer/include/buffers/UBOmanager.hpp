/** \file UBOmanager.hpp */
#pragma once
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

#include "rendering/uniformDataTypes.hpp"
#include "buffers/UBO.hpp"

/** \class UBOManager
*	\brief Holds UBO created when materails within a render pass are scanned*/
class UBOManager
{
public:
	UBOManager() = default; //!< Default constructor
	bool addUBO(const UBOLayout& layout); //!< Add a UBO

	template<typename T>
	bool setCachedValue(const std::string& blockName, const std::string& uniformName, T value); //!< Set a single cached value
	void uploadCachedValues() const; //!< Upload all cached values
private:
	template<typename T>
	bool uploadUniform(const std::string& blockName, const std::string& uniformName, T value) const; //!< Uploads a single value
	std::unordered_map<std::string, size_t> m_UBOMap; //!< Map of UBO names
	std::vector<std::shared_ptr<UBO>> m_UBOs; //!< UBOs being managed by this object
	//std::unordered_multimap<std::string, std::pair<std::string, UniformData>> m_UBODataCache; //!< INternal storage of data being uploaded

};

template<typename T>
inline bool UBOManager::setCachedValue(const std::string& blockName, const std::string& uniformName, T value)
{
	auto it = m_UBOMap.find(blockName);
	if (it != m_UBOMap.end())
	{
		auto& ubo = m_UBOs[it->second];
		auto layout = ubo->getLayout();
		for (auto& elem : layout)
		{
			if (elem.m_name == uniformName)
			{

				memcpy_s(ubo->m_dataBuffer.data() + elem.m_offset, elem.m_size, &value, elem.m_size);

				return true;
			}
		}
	}

	return false;
}

template<typename T>
inline bool UBOManager::uploadUniform(const std::string& blockName, const std::string& uniformName, T value) const
{
	bool result = false;

	auto it = m_UBOMap.find(blockName);
	if (it != m_UBOMap.end())
	{
		return m_UBOs[it->second]->uploadData(uniformName, (void*) (value));
	}
	
	return result;
}
