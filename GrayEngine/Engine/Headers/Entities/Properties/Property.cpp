#include "pch.h"
#include "Engine/Headers/Engine.h"
#include "Engine/Headers/Entities/Entity.h"
#include "Drawable.h"
#include "Engine/Headers/Entities/Skybox.h"
#include "Property.h"

////////////////////////////////////EntityID/////////////////////////////////////////////

EntityIDProperty::EntityIDProperty(UINT id, void* parent)
{
	property_value = id;
	property_name = "EntityID";
	property_type = PropertyType::EntityID;
	string_value = std::to_string(id);
	owner = parent;
}

EntityIDProperty::~EntityIDProperty()
{

}

const char* EntityIDProperty::ValueString()
{
	return string_value.c_str();
}

void EntityIDProperty::ParsePropertyValue(const char* value)
{
	//property_value = std::atoi(value);
	//string_value = value;
}

void EntityIDProperty::SetPropertyValue(UINT value)
{
	//property_value = value;
	//string_value = std::to_string(value);
}

UINT EntityIDProperty::GetValue()
{
	return property_value;
}

std::any EntityIDProperty::GetAnyValue()
{
	return property_value;
}

void* EntityIDProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Mass/////////////////////////////////////////////

MassProperty::MassProperty(float mass, void* parent)
{
	property_value = mass;
	property_name = "Mass";
	property_type = PropertyType::Mass;
	string_value = std::to_string(mass);
	owner = parent;
}

MassProperty::~MassProperty()
{

}

const char* MassProperty::ValueString()
{
	string_value = GrEngine::Globals::FloatToString(property_value, 5);
	return string_value.c_str();
}

void MassProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(std::stof(value));
}

void MassProperty::SetPropertyValue(float value)
{
	property_value = value;
}

std::any MassProperty::GetAnyValue()
{
	return property_value;
}

void* MassProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////EntityName/////////////////////////////////////////////

EntityNameProperty::EntityNameProperty(const char* name, void* parent)
{
	property_value = std::string(name);
	property_name = "EntityName";
	property_type = PropertyType::EntityName;
	owner = parent;
}

EntityNameProperty::~EntityNameProperty()
{

}

const char* EntityNameProperty::ValueString()
{
	return property_value.c_str();
}

void EntityNameProperty::ParsePropertyValue(const char* value)
{
	property_value = value;
}

void EntityNameProperty::SetPropertyValue(const char* value)
{
	property_value = value;
}

std::any EntityNameProperty::GetAnyValue()
{
	return property_value;
}

void* EntityNameProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Scale/////////////////////////////////////////////

ScaleProperty::ScaleProperty(float x, float y, float z, void* parent)
{
	property_value = { x, y, z };
	property_name = "Scale";
	property_type = PropertyType::Scale;
	owner = parent;
}

ScaleProperty::~ScaleProperty()
{

}

const char* ScaleProperty::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5));
	return property_string.c_str();
}

void ScaleProperty::ParsePropertyValue(const char* value)
{
	auto cols = GrEngine::Globals::SeparateString(value, ':');
	if (cols.size() < 3) return;

	property_value = { stof(cols[0]), stof(cols[1]), stof(cols[2]) };
}

void ScaleProperty::SetPropertyValue(const float& x, const float& y, const float& z)
{
	property_value = { x, y, z };
}

void ScaleProperty::SetPropertyValue(const glm::vec3& value)
{
	property_value = value;
}

std::any ScaleProperty::GetAnyValue()
{
	return property_value;
}

void* ScaleProperty::GetValueAdress()
{
	return &property_value;
}


////////////////////////////////////EntityPosition/////////////////////////////////////////////

EntityPositionProperty::EntityPositionProperty(float x, float y, float z, void* parent)
{
	property_value = { x, y, z };
	property_name = "EntityPosition";
	property_type = PropertyType::EntityPosition;
	owner = parent;
}

EntityPositionProperty::~EntityPositionProperty()
{

}

const char* EntityPositionProperty::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5));
	return property_string.c_str();
}

void EntityPositionProperty::ParsePropertyValue(const char* value)
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

void EntityPositionProperty::SetPropertyValue(const float& x, const float& y, const float& z)
{
	SetPropertyValue({ x, y, z });
}

void EntityPositionProperty::SetPropertyValue(const glm::vec3& value)
{
	property_value = value;
}

std::any EntityPositionProperty::GetAnyValue()
{
	return property_value;
}

void* EntityPositionProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////EntityOrientation/////////////////////////////////////////////

EntityOrientationProperty::EntityOrientationProperty(const float& pitch, const float& yaw, const float& roll, void* parent)
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

EntityOrientationProperty::~EntityOrientationProperty()
{

}

const char* EntityOrientationProperty::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(pitch_yaw_roll.x, 5) + ":" + GrEngine::Globals::FloatToString(pitch_yaw_roll.y, 5) + ":" + GrEngine::Globals::FloatToString(pitch_yaw_roll.z, 5));
	return property_string.c_str();
}

void EntityOrientationProperty::ParsePropertyValue(const char* degress)
{
	auto cols = GrEngine::Globals::SeparateString(degress, ':');

	if (cols.size() < 3) return;

	SetPropertyValue({ stof(cols[0]), stof(cols[1]), stof(cols[2]) });
	//static_cast<GrEngine::Entity*>(owner)->SetRotation(stof(cols[0]), stof(cols[1]), stof(cols[2]));
}

void EntityOrientationProperty::SetPropertyValue(glm::vec3 p_y_r)
{
	pitch_yaw_roll = p_y_r;
	glm::quat q = glm::quat_cast(glm::mat3(1.f));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.x), glm::vec3(1, 0, 0));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.y), glm::vec3(0, 1, 0));
	q = q * glm::angleAxis(glm::radians(pitch_yaw_roll.z), glm::vec3(0, 0, 1));
	property_value = q;
}

void EntityOrientationProperty::SetPropertyValue(glm::quat value)
{
	property_value = value;
	glm::quat q = glm::inverse(value);
	pitch_yaw_roll = glm::degrees(glm::eulerAngles(value));
}

std::any EntityOrientationProperty::GetAnyValue()
{
	return property_value;
}

void* EntityOrientationProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Color/////////////////////////////////////////////

ColorProperty::ColorProperty(void* parent)
{
	property_value = glm::vec4(1.f, 1.f, 1.f, 1.f);
	property_name = "Color";
	property_type = PropertyType::Color;
	owner = parent;
}

ColorProperty::ColorProperty(const float& r, const float& g, const float& b, const float& a, void* parent)
{
	property_value = glm::vec4(r, g, b, a);
	property_name = "Color";
	property_type = PropertyType::Color;
	owner = parent;
}

ColorProperty::ColorProperty(const float& r, const float& g, const float& b, void* parent)
{
	property_value = glm::vec4(r, g, b, 1.f);
	property_name = "Color";
	property_type = PropertyType::Color;
	owner = parent;
}

ColorProperty::~ColorProperty()
{

}

const char* ColorProperty::ValueString()
{
	property_string = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5) + ":" + GrEngine::Globals::FloatToString(property_value.w, 5));
	return property_string.c_str();
}

void ColorProperty::ParsePropertyValue(const char* value)
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

void ColorProperty::SetPropertyValue(const float& r, const float& g, const float& b, const float& a)
{
	property_value = glm::vec4(r, g, b, a);
}

void ColorProperty::SetPropertyValue(glm::vec4 value)
{
	property_value = value;
}

std::any ColorProperty::GetAnyValue()
{
	return property_value;
}

void* ColorProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////PhysComponent/////////////////////////////////////////////

PhysComponentProperty::PhysComponentProperty(void* parent)
{
	property_name = "PhysComponent";
	property_type = PropertyType::PhysComponent;
	owner = parent;
	phys = GrEngine::Engine::GetContext()->GetPhysics()->InitSimulationObject(static_cast<GrEngine::Entity*>(owner));
}

PhysComponentProperty::~PhysComponentProperty()
{
	GrEngine::Engine::GetContext()->GetPhysics()->RemoveSimulationObject(static_cast<GrEngine::PhysicsObject*>(phys));
}

const char* PhysComponentProperty::ValueString()
{
	property_string = std::to_string(property_value);
	return property_string.c_str();
}

void PhysComponentProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(std::atoi(value));
}

void PhysComponentProperty::SetPropertyValue(int value)
{
	property_value = value;
	static_cast<GrEngine::PhysicsObject*>(phys)->SetKinematic(value);
}

std::any PhysComponentProperty::GetAnyValue()
{
	return static_cast<GrEngine::PhysicsObject*>(phys);
}

void* PhysComponentProperty::GetValueAdress()
{
	return phys;
}

////////////////////////////////////CollisionType/////////////////////////////////////////////

CollisionTypeProperty::CollisionTypeProperty(void* parent)
{
	property_name = "CollisionType";
	property_type = PropertyType::CollisionType;
	owner = parent;
}

CollisionTypeProperty::~CollisionTypeProperty()
{
}

const char* CollisionTypeProperty::ValueString()
{
	property_string = std::to_string(property_value);
	return property_string.c_str();
}

void CollisionTypeProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(std::atoi(value));
}

void CollisionTypeProperty::SetPropertyValue(int value)
{
	property_value = value;
	GrEngine::PhysicsObject* comp = static_cast<GrEngine::Entity*>(owner)->GetPropertyValue(PropertyType::PhysComponent, static_cast<GrEngine::PhysicsObject*>(nullptr));
	if (comp != nullptr)
	{
		comp->UpdateCollisionType((CollisionTypeEnum)value);
	}
}

std::any CollisionTypeProperty::GetAnyValue()
{
	return property_value;
}

void* CollisionTypeProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Drawable/////////////////////////////////////////////

DrawableProperty::DrawableProperty(const char* path, void* parent)
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

DrawableProperty::~DrawableProperty()
{

}

const char* DrawableProperty::ValueString()
{
	return static_cast<GrEngine::Object*>(drawable)->gmf_name.c_str();
}

void DrawableProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void DrawableProperty::SetPropertyValue(std::string value)
{
	property_value = value;

	if (owner != nullptr && value != "nil")
		GrEngine::Engine::GetContext()->LoadFromGMF(static_cast<GrEngine::Entity*>(owner)->GetEntityID(), value.c_str());
}

std::any DrawableProperty::GetAnyValue()
{
	return static_cast<GrEngine::Object*>(drawable);
}

void* DrawableProperty::GetValueAdress()
{
	return drawable;
}

////////////////////////////////////Spotlight/////////////////////////////////////////////

SpotLightProperty::SpotLightProperty(void* parent)
{
	property_name = "Spotlight";
	property_type = PropertyType::Spotlight;
	owner = parent;
	spotlight = GrEngine::Engine::GetContext()->GetRenderer()->InitSpotlightObject(static_cast<GrEngine::Entity*>(owner));
}

SpotLightProperty::~SpotLightProperty()
{

}

const char* SpotLightProperty::ValueString()
{
	return property_value.c_str();
}

void SpotLightProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void SpotLightProperty::SetPropertyValue(std::string value)
{
	property_value = value;
}

std::any SpotLightProperty::GetAnyValue()
{
	return static_cast<GrEngine::LightObject*>(spotlight);
}

void* SpotLightProperty::GetValueAdress()
{
	return spotlight;
}

////////////////////////////////////Cascade light/////////////////////////////////////////////

CascadeProperty::CascadeProperty(void* parent)
{
	property_name = "CascadeLight";
	property_type = PropertyType::CascadeLight;
	owner = parent;
	cascade = GrEngine::Engine::GetContext()->GetRenderer()->InitCascadeLightObject(static_cast<GrEngine::Entity*>(owner));
}

CascadeProperty::~CascadeProperty()
{

}

const char* CascadeProperty::ValueString()
{
	return property_value.c_str();
}

void CascadeProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void CascadeProperty::SetPropertyValue(std::string value)
{
	property_value = value;
}

std::any CascadeProperty::GetAnyValue()
{
	return static_cast<GrEngine::LightObject*>(cascade);
}

void* CascadeProperty::GetValueAdress()
{
	return cascade;
}

////////////////////////////////////Point light/////////////////////////////////////////////

PointLightPropery::PointLightPropery(void* parent)
{
	property_name = "PointLight";
	property_type = PropertyType::PointLight;
	owner = parent;
	point = GrEngine::Engine::GetContext()->GetRenderer()->InitPointLightObject(static_cast<GrEngine::Entity*>(owner));
}

PointLightPropery::~PointLightPropery()
{

}

const char* PointLightPropery::ValueString()
{
	return property_value.c_str();
}

void PointLightPropery::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void PointLightPropery::SetPropertyValue(std::string value)
{
	property_value = value;
}

std::any PointLightPropery::GetAnyValue()
{
	return static_cast<GrEngine::LightObject*>(point);
}

void* PointLightPropery::GetValueAdress()
{
	return point;
}

////////////////////////////////////Omni light/////////////////////////////////////////////

OmniLightPropery::OmniLightPropery(void* parent)
{
	property_name = "OmniLight";
	property_type = PropertyType::OmniLight;
	owner = parent;
	omni = GrEngine::Engine::GetContext()->GetRenderer()->InitOmniLightObject(static_cast<GrEngine::Entity*>(owner));
}

OmniLightPropery::~OmniLightPropery()
{

}

const char* OmniLightPropery::ValueString()
{
	return property_value.c_str();
}

void OmniLightPropery::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void OmniLightPropery::SetPropertyValue(std::string value)
{
	property_value = value;
}

std::any OmniLightPropery::GetAnyValue()
{
	return static_cast<GrEngine::LightObject*>(omni);
}

void* OmniLightPropery::GetValueAdress()
{
	return omni;
}

////////////////////////////////////Cubemap Property/////////////////////////////////////////////

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

ShaderProperty::ShaderProperty(const char* path, void* parent)
{
	property_name = "Shader";
	property_value = path;
	property_type = PropertyType::Shader;
	owner = parent;
}

ShaderProperty::~ShaderProperty()
{

}

const char* ShaderProperty::ValueString()
{
	return property_value.c_str();
}

void ShaderProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(value);
}

void ShaderProperty::SetPropertyValue(std::string value)
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

std::any ShaderProperty::GetAnyValue()
{
	return property_value;
}

void* ShaderProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Transparency/////////////////////////////////////////////

TransparencyProperty::TransparencyProperty(bool value, void* parent)
{
	property_name = "Transparency";
	property_value = value;
	property_string = std::to_string((int)value);
	property_type = PropertyType::Transparency;
	owner = parent;
}

TransparencyProperty::~TransparencyProperty()
{

}

const char* TransparencyProperty::ValueString()
{
	return property_string.c_str();
}

void TransparencyProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(atoi(value));
	property_string = value;
}

void TransparencyProperty::SetPropertyValue(bool value)
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

std::any TransparencyProperty::GetAnyValue()
{
	return property_value;
}

void* TransparencyProperty::GetValueAdress()
{
	return &property_value;
}


////////////////////////////////////Double sided/////////////////////////////////////////////

DoubleSidedProperty::DoubleSidedProperty(bool value, void* parent)
{
	property_name = "DoubleSided";
	property_value = value;
	property_string = std::to_string((int)value);
	property_type = PropertyType::DoubleSided;
	owner = parent;
}

DoubleSidedProperty::~DoubleSidedProperty()
{

}

const char* DoubleSidedProperty::ValueString()
{
	return property_string.c_str();
}

void DoubleSidedProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(atoi(value));
	property_string = value;
}

void DoubleSidedProperty::SetPropertyValue(bool value)
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

std::any DoubleSidedProperty::GetAnyValue()
{
	return property_value;
}

void* DoubleSidedProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Cast shadow/////////////////////////////////////////////

CastShadowProperty::CastShadowProperty(bool value, void* parent)
{
	property_name = "CastShadow";
	property_value = value;
	property_type = PropertyType::CastShadow;
	owner = parent;
}

CastShadowProperty::~CastShadowProperty()
{

}

const char* CastShadowProperty::ValueString()
{
	return property_string.c_str();
}

void CastShadowProperty::ParsePropertyValue(const char* value)
{
	SetPropertyValue(atoi(value));
	property_string = value;
}

void CastShadowProperty::SetPropertyValue(int value)
{
	property_value = value;
	property_string = std::to_string(value);
}

std::any CastShadowProperty::GetAnyValue()
{
	return property_value;
}

void* CastShadowProperty::GetValueAdress()
{
	return &property_value;
}