#include "pch.h"
#include "Engine/Headers/Engine.h"
#include "Engine/Headers/Entities/Entity.h"
#include "Drawable.h"
#include "Engine/Headers/Entities/Skybox.h"
#include "Property.h"

////////////////////////////////////EntityID/////////////////////////////////////////////

EntityID::EntityID(UINT id, void* parent)
{
	property_value = id;
	property_name = "EntityID";
	property_type = PropertyType::EntityID;
	string_value = std::to_string(id);
	owner = parent;
}

EntityID::~EntityID()
{

}

const char* EntityID::ValueString()
{
	return string_value.c_str();
}

void EntityID::ParsePropertyValue(const char* value)
{
	//property_value = std::atoi(value);
	//string_value = value;
}

void EntityID::SetPropertyValue(UINT value)
{
	//property_value = value;
	//string_value = std::to_string(value);
}

UINT EntityID::GetValue()
{
	return property_value;
}

std::any EntityID::GetAnyValue()
{
	return property_value;
}

void* EntityID::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Mass/////////////////////////////////////////////

Mass::Mass(float mass, void* parent)
{
	property_value = mass;
	property_name = "Mass";
	property_type = PropertyType::Mass;
	string_value = std::to_string(mass);
	owner = parent;
}

Mass::~Mass()
{

}

const char* Mass::ValueString()
{
	string_value = GrEngine::Globals::FloatToString(property_value, 5);
	return string_value.c_str();
}

void Mass::ParsePropertyValue(const char* value)
{
	SetPropertyValue(std::stof(value));
}

void Mass::SetPropertyValue(float value)
{
	property_value = value;
}

std::any Mass::GetAnyValue()
{
	return property_value;
}

void* Mass::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////EntityName/////////////////////////////////////////////

EntityName::EntityName(const char* name, void* parent)
{
	property_value = std::string(name);
	property_name = "EntityName";
	property_type = PropertyType::EntityName;
	owner = parent;
}

EntityName::~EntityName()
{

}

const char* EntityName::ValueString()
{
	return property_value.c_str();
}

void EntityName::ParsePropertyValue(const char* value)
{
	property_value = value;
}

void EntityName::SetPropertyValue(const char* value)
{
	property_value = value;
}

std::any EntityName::GetAnyValue()
{
	return property_value;
}

void* EntityName::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Scale/////////////////////////////////////////////

Scale::Scale(float x, float y, float z, void* parent)
{
	property_value = { x, y, z };
	property_name = "Scale";
	property_type = PropertyType::Scale;
	owner = parent;
}

Scale::~Scale()
{

}

const char* Scale::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5));
	return property_string.c_str();
}

void Scale::ParsePropertyValue(const char* value)
{
	auto cols = GrEngine::Globals::SeparateString(value, ':');
	if (cols.size() < 3) return;

	property_value = { stof(cols[0]), stof(cols[1]), stof(cols[2]) };
}

void Scale::SetPropertyValue(const float& x, const float& y, const float& z)
{
	property_value = { x, y, z };
}

void Scale::SetPropertyValue(const glm::vec3& value)
{
	property_value = value;
}

std::any Scale::GetAnyValue()
{
	return property_value;
}

void* Scale::GetValueAdress()
{
	return &property_value;
}


////////////////////////////////////EntityPosition/////////////////////////////////////////////

EntityPosition::EntityPosition(float x, float y, float z, void* parent)
{
	property_value = { x, y, z };
	property_name = "EntityPosition";
	property_type = PropertyType::EntityPosition;
	owner = parent;
}

EntityPosition::~EntityPosition()
{

}

const char* EntityPosition::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5));
	return property_string.c_str();
}

void EntityPosition::ParsePropertyValue(const char* value)
{
	auto cols = GrEngine::Globals::SeparateString(value, ':');
	if (cols.size() < 3) return;
	glm::vec3 res{};

	for (int ind = 0; ind < cols.size(); ind++)
	{
		try
		{
			res[ind] = std::stof(cols[ind]);
		}
		catch (...)
		{
			res[ind] = property_value[ind];
		}
	}
	
	static_cast<GrEngine::Entity*>(owner)->PositionObjectAt(res);
}

void EntityPosition::SetPropertyValue(const float& x, const float& y, const float& z)
{
	SetPropertyValue({ x, y, z });
}

void EntityPosition::SetPropertyValue(const glm::vec3& value)
{
	property_value = value;
}

std::any EntityPosition::GetAnyValue()
{
	return property_value;
}

void* EntityPosition::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////EntityOrientation/////////////////////////////////////////////

EntityOrientation::EntityOrientation(const float& pitch, const float& yaw, const float& roll, void* parent)
{
	pitch_yaw_roll = { pitch, yaw, roll };
	glm::quat q = glm::quat_cast(glm::mat3(1.f));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
	property_value = q;

	property_name = "EntityOrientation";
	property_type = PropertyType::EntityOrientation;
	owner = parent;
}

EntityOrientation::~EntityOrientation()
{

}

const char* EntityOrientation::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(pitch_yaw_roll.x, 5) + ":" + GrEngine::Globals::FloatToString(pitch_yaw_roll.y, 5) + ":" + GrEngine::Globals::FloatToString(pitch_yaw_roll.z, 5));
	return property_string.c_str();
}

void EntityOrientation::ParsePropertyValue(const char* degress)
{
	auto cols = GrEngine::Globals::SeparateString(degress, ':');

	if (cols.size() < 3) return;

	SetPropertyValue({ stof(cols[0]), stof(cols[1]), stof(cols[2]) });
	//static_cast<GrEngine::Entity*>(owner)->SetRotation(stof(cols[0]), stof(cols[1]), stof(cols[2]));
}

void EntityOrientation::SetPropertyValue(glm::vec3 p_y_r)
{
	pitch_yaw_roll = p_y_r;
	glm::quat q = glm::quat_cast(glm::mat3(1.f));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
	property_value = q;
}

void EntityOrientation::SetPropertyValue(glm::quat value)
{
	property_value = value;
	glm::quat q = glm::inverse(value);
	pitch_yaw_roll = glm::degrees(glm::eulerAngles(value));
}

std::any EntityOrientation::GetAnyValue()
{
	return property_value;
}

void* EntityOrientation::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Color/////////////////////////////////////////////

Color::Color(void* parent)
{
	property_value = glm::vec4(1.f, 1.f, 1.f, 1.f);
	property_name = "Color";
	property_type = PropertyType::Color;
	owner = parent;
}

Color::Color(const float& r, const float& g, const float& b, const float& a, void* parent)
{
	property_value = glm::vec4(r, g, b, a);
	property_name = "Color";
	property_type = PropertyType::Color;
	owner = parent;
}

Color::Color(const float& r, const float& g, const float& b, void* parent)
{
	property_value = glm::vec4(r, g, b, 1.f);
	property_name = "Color";
	property_type = PropertyType::Color;
	owner = parent;
}

Color::~Color()
{

}

const char* Color::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5) + ":" + GrEngine::Globals::FloatToString(property_value.w, 5));
	return property_string.c_str();
}

void Color::ParsePropertyValue(const char* value)
{
	auto cols = GrEngine::Globals::SeparateString(value, ':');

	if (cols.size() < 3) return;

	if (cols.size() == 3)
	{
		SetPropertyValue(stof(cols[0]), stof(cols[1]), stof(cols[2]), 1.f);
	}
	else
	{
		SetPropertyValue(stof(cols[0]), stof(cols[1]), stof(cols[2]), stof(cols[3]));
	}
}

void Color::SetPropertyValue(const float& r, const float& g, const float& b, const float& a)
{
	property_value = glm::vec4(r, g, b, a);
}

void Color::SetPropertyValue(glm::vec4 value)
{
	property_value = value;
}

std::any Color::GetAnyValue()
{
	return property_value;
}

void* Color::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////PhysComponent/////////////////////////////////////////////

PhysComponent::PhysComponent(void* parent)
{
	property_name = "PhysComponent";
	property_type = PropertyType::PhysComponent;
	owner = parent;
	phys = GrEngine::Engine::GetContext()->GetPhysics()->InitSimulationObject(static_cast<GrEngine::Entity*>(owner));
}

PhysComponent::~PhysComponent()
{
	GrEngine::Engine::GetContext()->GetPhysics()->RemoveSimulationObject(static_cast<GrEngine::PhysicsObject*>(phys));
}

const char* PhysComponent::ValueString()
{
	property_string = std::to_string(property_value);
	return property_string.c_str();
}

void PhysComponent::ParsePropertyValue(const char* value)
{
	SetPropertyValue(std::atoi(value));
}

void PhysComponent::SetPropertyValue(int value)
{
	property_value = value;
	static_cast<GrEngine::PhysicsObject*>(phys)->SetKinematic(value);
}

std::any PhysComponent::GetAnyValue()
{
	return static_cast<GrEngine::PhysicsObject*>(phys);
}

void* PhysComponent::GetValueAdress()
{
	return phys;
}

////////////////////////////////////CollisionType/////////////////////////////////////////////

CollisionType::CollisionType(void* parent)
{
	property_name = "CollisionType";
	property_type = PropertyType::CollisionType;
	owner = parent;
}

CollisionType::~CollisionType()
{
}

const char* CollisionType::ValueString()
{
	property_string = std::to_string(property_value);
	return property_string.c_str();
}

void CollisionType::ParsePropertyValue(const char* value)
{
	SetPropertyValue(std::atoi(value));
}

void CollisionType::SetPropertyValue(int value)
{
	property_value = value;
	GrEngine::PhysicsObject* comp = static_cast<GrEngine::Entity*>(owner)->GetPropertyValue(PropertyType::PhysComponent, static_cast<GrEngine::PhysicsObject*>(nullptr));
	if (comp != nullptr)
	{
		comp->UpdateCollisionType((CollisionTypeEnum)value);
	}
}

std::any CollisionType::GetAnyValue()
{
	return property_value;
}

void* CollisionType::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Drawable/////////////////////////////////////////////

Drawable::Drawable(const char* path, void* parent)
{
	property_name = "Drawable";
	property_type = PropertyType::Drawable;
	owner = parent;
	drawable = GrEngine::Engine::GetContext()->GetRenderer()->InitDrawableObject(static_cast<GrEngine::Entity*>(owner));

	if (path != "" && path != "nil")
	{
		GrEngine::Engine::GetContext()->LoadFromGMF(static_cast<GrEngine::Entity*>(owner)->GetEntityID(), path);
	}
}

Drawable::~Drawable()
{

}

const char* Drawable::ValueString()
{
	return property_value.c_str();
}

void Drawable::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void Drawable::SetPropertyValue(std::string value)
{
	property_value = value;

	if (owner != nullptr && value != "nil")
		GrEngine::Engine::GetContext()->LoadFromGMF(static_cast<GrEngine::Entity*>(owner)->GetEntityID(), value.c_str());
}

std::any Drawable::GetAnyValue()
{
	return static_cast<GrEngine::Object*>(drawable);
}

void* Drawable::GetValueAdress()
{
	return drawable;
}

////////////////////////////////////Spotlight/////////////////////////////////////////////

SpotLight::SpotLight(void* parent)
{
	property_name = "Spotlight";
	property_type = PropertyType::Spotlight;
	owner = parent;
	spotlight = GrEngine::Engine::GetContext()->GetRenderer()->InitSpotlightObject(static_cast<GrEngine::Entity*>(owner));
}

SpotLight::~SpotLight()
{

}

const char* SpotLight::ValueString()
{
	return property_value.c_str();
}

void SpotLight::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void SpotLight::SetPropertyValue(std::string value)
{
	property_value = value;
}

std::any SpotLight::GetAnyValue()
{
	return spotlight;
}

void* SpotLight::GetValueAdress()
{
	return spotlight;
}

////////////////////////////////////CubemapProperty/////////////////////////////////////////////

CubemapProperty::CubemapProperty(std::array<std::string, 6> textures, void* parent)
{
	property_name = "CubemapProperty";
	property_value = textures;
	property_type = PropertyType::Cubemap;
	owner = parent;
}

CubemapProperty::~CubemapProperty()
{

}

const char* CubemapProperty::ValueString()
{
	for (int i = 0; i < 6; i++)
	{
		property_string += property_value[i] + '|';
	}

	return property_string.c_str();
}

void CubemapProperty::ParsePropertyValue(const char* value)
{
	auto texs = GrEngine::Globals::SeparateString(value, '|');
	std::copy_n(texs.begin(), 6, property_value.begin());

	if (GrEngine::Globals::VectorContains<std::string>(texs, ""))
		return;

	if (owner != nullptr)
		static_cast<GrEngine::Skybox*>(owner)->UpdateTextures(property_value);
}

void CubemapProperty::SetPropertyValue(std::array<std::string, 6> value)
{
	property_value = value;

	if (owner != nullptr)
		static_cast<GrEngine::Skybox*>(owner)->UpdateTextures(property_value);
}

std::any CubemapProperty::GetAnyValue()
{
	return property_value;
}

void* CubemapProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Shader/////////////////////////////////////////////

Shader::Shader(const char* path, void* parent)
{
	property_name = "Shader";
	property_value = path;
	property_type = PropertyType::Shader;
	owner = parent;
}

Shader::~Shader()
{

}

const char* Shader::ValueString()
{
	return property_value.c_str();
}

void Shader::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void Shader::SetPropertyValue(std::string value)
{
	property_value = value;

	if (owner != nullptr)
	{
		GrEngine::Object* mesh = GrEngine::Object::FindObject(static_cast<GrEngine::Entity*>(owner));
		if (mesh != nullptr)
		{
			mesh->Refresh();
		}
	}
}

std::any Shader::GetAnyValue()
{
	return property_value;
}

void* Shader::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Transparency/////////////////////////////////////////////

Transparency::Transparency(bool value, void* parent)
{
	property_name = "Transparency";
	property_value = value;
	property_string = std::to_string((int)value);
	property_type = PropertyType::Transparency;
	owner = parent;
}

Transparency::~Transparency()
{

}

const char* Transparency::ValueString()
{
	return property_string.c_str();
}

void Transparency::ParsePropertyValue(const char* value)
{
	SetPropertyValue(atoi(value));
	property_string = value;
}

void Transparency::SetPropertyValue(bool value)
{
	property_value = value;
	property_string = std::to_string(value);

	if (owner != nullptr)
	{
		GrEngine::Object* mesh = GrEngine::Object::FindObject(static_cast<GrEngine::Entity*>(owner));
		if (mesh != nullptr)
		{
			mesh->Refresh();
		}
	}
}

std::any Transparency::GetAnyValue()
{
	return property_value;
}

void* Transparency::GetValueAdress()
{
	return &property_value;
}


////////////////////////////////////Double sided/////////////////////////////////////////////

DoubleSided::DoubleSided(bool value, void* parent)
{
	property_name = "DoubleSided";
	property_value = value;
	property_string = std::to_string((int)value);
	property_type = PropertyType::DoubleSided;
	owner = parent;
}

DoubleSided::~DoubleSided()
{

}

const char* DoubleSided::ValueString()
{
	return property_string.c_str();
}

void DoubleSided::ParsePropertyValue(const char* value)
{
	SetPropertyValue(atoi(value));
	property_string = value;
}

void DoubleSided::SetPropertyValue(bool value)
{
	property_value = value;
	property_string = std::to_string(value);

	if (owner != nullptr)
	{
		GrEngine::Object* mesh = GrEngine::Object::FindObject(static_cast<GrEngine::Entity*>(owner));
		if (mesh != nullptr)
		{
			mesh->Refresh();
		}
	}
}

std::any DoubleSided::GetAnyValue()
{
	return property_value;
}

void* DoubleSided::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Cast shadow/////////////////////////////////////////////

CastShadow::CastShadow(bool value, void* parent)
{
	property_name = "CastShadow";
	property_value = value;
	property_type = PropertyType::CastShadow;
	owner = parent;
}

CastShadow::~CastShadow()
{

}

const char* CastShadow::ValueString()
{
	return property_string.c_str();
}

void CastShadow::ParsePropertyValue(const char* value)
{
	SetPropertyValue(atoi(value));
	property_string = value;
}

void CastShadow::SetPropertyValue(int value)
{
	property_value = value;
	property_string = std::to_string(value);
}

std::any CastShadow::GetAnyValue()
{
	return property_value;
}

void* CastShadow::GetValueAdress()
{
	return &property_value;
}