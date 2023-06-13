#include "pch.h"
#include "Entities/Entity.h"
#include "Entities/Properties/Drawable.h"
#include "Engine/Headers/Engine.h"

namespace GrEngine
{

	EntityProperty* Entity::AddNewProperty(PropertyType type)
	{
		if (!HasProperty(type))
		{
			switch (type)
			{
			case PropertyType::Error:
				break;
			case PropertyType::EntityID:
				break;
			case PropertyType::Mass:
				properties.push_back(new FloatProperty(PropertyType::Mass, 0.f, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							PhysicsObject* comp = (PhysicsObject*)owner->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);
							if (comp != nullptr)
							{
								comp->Dispose();
								comp->CalculatePhysics();
							}
						}
					});
				return properties.back();
			case PropertyType::EntityName:
				break;
			case PropertyType::Scale:
				properties.push_back(new Vector3fProperty(PropertyType::Scale, 1.f, 1.f, 1.f, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							PhysicsObject* comp = (PhysicsObject*)owner->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);
							if (comp != nullptr)
							{
								comp->UpdateCollisionType(owner->GetPropertyValue(PropertyType::CollisionType, 0));
							}
						}
					});
				return properties.back();
			case PropertyType::EntityPosition:
				break;
			case PropertyType::EntityOrientation:
				break;
			case PropertyType::Color:
				properties.push_back(new Vector4fProperty(PropertyType::Color, 1.f, 1.f, 1.f, 1.f, this));
				return properties.back();
			case PropertyType::PhysComponent:
				properties.push_back(new PointerProperty(PropertyType::PhysComponent, Engine::GetContext()->GetPhysics()->InitSimulationObject(this), [](Entity* owner, EntityProperty* self) {
						Engine::GetContext()->GetPhysics()->RemoveSimulationObject(static_cast<PhysicsObject*>(self->GetValueAdress()));
					}, this));
				physComp = static_cast<PhysicsObject*>(properties.back()->GetValueAdress());
				return properties.back();
			case PropertyType::BodyType:
				properties.push_back(new IntegerProperty(PropertyType::BodyType, 0, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							PhysicsObject* comp = (PhysicsObject*)owner->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);
							if (comp != nullptr)
							{
								comp->SetKinematic(*static_cast<int*>(self->GetValueAdress()));
							}
						}
					});
				return properties.back();
			case PropertyType::Drawable:
				properties.push_back(new PointerProperty(PropertyType::Drawable, Engine::GetContext()->GetRenderer()->InitDrawableObject(this), [](Entity* owner, EntityProperty* self) {

					}, this));
				Type |= EntityType::ObjectEntity;
				return properties.back();
			case PropertyType::Spotlight:
				properties.push_back(new PointerProperty(PropertyType::Spotlight, Engine::GetContext()->GetRenderer()->InitSpotlightObject(this), [](Entity* owner, EntityProperty* self) {
					
					}, this));
				Type |= EntityType::SpotlightEntity;
				return properties.back();
			case PropertyType::CascadeLight:
				properties.push_back(new PointerProperty(PropertyType::CascadeLight, Engine::GetContext()->GetRenderer()->InitCascadeLightObject(this), [](Entity* owner, EntityProperty* self) {

					}, this));
				Type |= EntityType::CascadeLightEntity;
				return properties.back();
			case PropertyType::PointLight:
				properties.push_back(new PointerProperty(PropertyType::PointLight, Engine::GetContext()->GetRenderer()->InitPointLightObject(this), [](Entity* owner, EntityProperty* self) {

					}, this));
				Type |= EntityType::PointLightEntity;
				return properties.back();
			case PropertyType::OmniLight:
				properties.push_back(new PointerProperty(PropertyType::OmniLight, Engine::GetContext()->GetRenderer()->InitOmniLightObject(this), [](Entity* owner, EntityProperty* self) {

					}, this));
				Type |= EntityType::OmniLightEntity;
				return properties.back();
			case PropertyType::Cubemap:
				break;
			case PropertyType::Shader:
				properties.push_back(new StringProperty(PropertyType::Shader, "Shaders//default", this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							Object* mesh = Object::FindObject(owner);
							if (mesh != nullptr)
							{
								mesh->Refresh();
							}
						}
					});
				return properties.back();
			case PropertyType::Transparency:
				properties.push_back(new IntegerProperty(PropertyType::Transparency, 0, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							Object* mesh = Object::FindObject(owner);
							if (mesh != nullptr)
							{
								mesh->Refresh();
							}
						}
					});
				return properties.back();
			case PropertyType::DoubleSided:
				properties.push_back(new IntegerProperty(PropertyType::DoubleSided, 0, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							Object* mesh = Object::FindObject(owner);
							if (mesh != nullptr)
							{
								mesh->Refresh();
							}
						}
					});
				return properties.back();
			case PropertyType::CastShadow:
				properties.push_back(new IntegerProperty(PropertyType::CastShadow, 1, this));
				return properties.back();
			case PropertyType::CollisionType:
				properties.push_back(new IntegerProperty(PropertyType::CollisionType, 0, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							PhysicsObject* comp = (PhysicsObject*)owner->GetPropertyValue(PropertyType::PhysComponent, (void*)nullptr);
							if (comp != nullptr)
							{
								comp->UpdateCollisionType((int)*static_cast<int*>(self->GetValueAdress()));
							}
						}
					});
				return properties.back();
			case PropertyType::ModelPath:
				properties.push_back(new StringProperty(PropertyType::ModelPath, "", this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							Engine::GetContext()->LoadFromGMF(owner->GetEntityID(), self->ValueString());
						}
					});
				return properties.back();
			case PropertyType::AlphaThreshold:
				properties.push_back(new FloatProperty(PropertyType::AlphaThreshold, 0.5f, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							Object* mesh = Object::FindObject(owner);
							if (mesh != nullptr)
							{
								mesh->Refresh();
							}
						}
					});
				return properties.back();
			case PropertyType::MaximumDistance:
				properties.push_back(new FloatProperty(PropertyType::MaximumDistance, -1.f, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							Object* mesh = Object::FindObject(owner);
							if (mesh != nullptr)
							{
								mesh->Refresh();
							}
							void* light = owner->GetPropertyValue(PropertyType::PointLight, (void*)nullptr);
							light = light == nullptr ? owner->GetPropertyValue(PropertyType::OmniLight, (void*)nullptr) : light;
							light = light == nullptr ? owner->GetPropertyValue(PropertyType::Spotlight, (void*)nullptr) : light;
							if (light != nullptr)
							{
								static_cast<LightObject*>(light)->UpdateLight();
							}
						}
					});
				return properties.back();
			case PropertyType::Brightness:
				properties.push_back(new FloatProperty(PropertyType::Brightness, 1.f, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							void* light = owner->GetPropertyValue(PropertyType::PointLight, (void*)nullptr);
							light = light == nullptr ? owner->GetPropertyValue(PropertyType::OmniLight, (void*)nullptr) : light;
							light = light == nullptr ? owner->GetPropertyValue(PropertyType::Spotlight, (void*)nullptr) : light;
							if (light != nullptr)
							{
								static_cast<LightObject*>(light)->UpdateLight();
							}
						}
					});
				return properties.back();
			case PropertyType::PlayerController:
				properties.push_back(new PointerProperty(PropertyType::PlayerController, Engine::GetContext()->GetPhysics()->InitController(this), [](Entity* owner, EntityProperty* self) {
					Engine::GetContext()->GetPhysics()->RemoveController(static_cast<MovementComponent*>(self->GetValueAdress()));
					}, this));
				return properties.back();
			case PropertyType::NormalStrength:
				properties.push_back(new FloatProperty(PropertyType::NormalStrength, 1.0f, this));
				properties.back()->SetCallback([](Entity* owner, EntityProperty* self)
					{
						if (owner != nullptr)
						{
							Object* mesh = Object::FindObject(owner);
							if (mesh != nullptr)
							{
								mesh->Refresh();
							}
						}
					});
				return properties.back();
			default:
				break;
			}
		}
		else
		{
			return GetProperty(type);
		}
	}
};