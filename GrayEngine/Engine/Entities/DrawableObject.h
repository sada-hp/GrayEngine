#pragma once
#include <pch.h>
#include "Engine/Source/Headers/Entity.h"

namespace GrEngine
{
	struct Texture
	{
		const char* texture_path;
	};

	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 color;
		glm::vec2 uv;
		uint32_t uv_index;

		bool operator==(const Vertex& other) const
		{
			return pos == other.pos && uv == other.uv;
		}
	};

	struct Mesh
	{
		const char* mesh_path;
	};

	class DllExport DrawableObject : public Entity
	{
	public:
		DrawableObject() {};
		virtual ~DrawableObject() {};


		glm::vec3& GetObjectBounds()
		{
			return bound;
		}

		void SetObjectBounds(glm::vec3 new_bounds)
		{
			bound = new_bounds;
		}

	private:
		glm::vec3 bound = { 0.f, 0.f, 0.f };
	};
}
