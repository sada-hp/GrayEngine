#pragma once
#include <pch.h>
#include "Entity.h"
#include "Engine/Headers/Engine.h"
#include "Engine/Headers/Virtual/Physics.h"

namespace GrEngine
{
	class Terrain : public Entity
	{
	public:
		struct TerrainSize
		{
			int resolution;
			int width;
			int depth;
			int height;
		};

		Terrain()
		{
			Type |= EntityType::TerrainEntity;
		}

		Terrain(UINT ID) : Entity(ID)
		{
			Type |= EntityType::TerrainEntity;
		}

		virtual ~Terrain()
		{
			if (physComponent != nullptr)
			{
				physComponent->Dispose();
				delete physComponent;
			}
		}

		virtual void GenerateTerrain(int resolution, int width, int height, int depth, std::array<std::string, 6> images, std::array<std::string, 4> normals, std::array<std::string, 4> displacement) = 0;
		virtual void calculateCollisions() = 0;
		virtual void UpdateCollision() = 0;
		virtual void UpdateFoliageMask(void* pixels) = 0;
		virtual void UpdateFoliageMask(void* pixels, uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y) = 0;
		virtual void UpdateTextures(std::array<std::string, 5> images, std::array<std::string, 4> normals, std::array<std::string, 4> displacement) = 0;
		virtual void OffsetVertices(std::map<UINT, float> offsets) = 0;
		virtual void UpdateVertices(std::map<UINT, float> offsets) = 0;
		virtual void SaveTerrain(const char* filepath) = 0;
		virtual bool LoadTerrain(const char* filepath) = 0;
		virtual glm::vec4& GetVertexPosition(UINT pos) = 0;
		virtual const std::string GetBlendMask() = 0;
		virtual const std::array<std::string, 4> GetColorTextures() = 0;
		virtual const std::array<std::string, 4> GetNormalTextures() = 0;
		virtual const std::array<std::string, 4> GetDisplacementTextures() = 0;
		TerrainSize& GetTerrainSize() { return size; }

		PhysicsObject* physComponent;
	protected:
		TerrainSize size;
	};
}