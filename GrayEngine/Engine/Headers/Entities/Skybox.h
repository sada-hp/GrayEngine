#pragma once
#include <pch.h>
#include "Entity.h"

namespace GrEngine
{
	class DllExport Skybox : public Entity
	{
	public:
		Skybox()
		{
			Type |= EntityType::SkyboxEntity;
			prop = new CubemapProperty({"","","","","",""}, this);
			properties.push_back(prop);
		};

		Skybox(UINT id) : Entity(id)
		{
			Type |= EntityType::SkyboxEntity;
			prop = new CubemapProperty({ "","","","","","" }, this);
			properties.push_back(prop);
		};

		virtual ~Skybox()
		{
		};

		virtual bool AssignTextures(std::array<std::string, 6> sky) = 0;

	private:
		CubemapProperty* prop;
	};
}