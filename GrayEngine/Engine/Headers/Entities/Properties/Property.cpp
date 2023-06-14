#include "pch.h"
#include "Engine/Headers/Engine.h"
#include "Engine/Headers/Entities/Entity.h"
#include "Drawable.h"
#include "Engine/Headers/Entities/Skybox.h"
#include "Property.h"

StringProperty::StringProperty(const char* name, std::string value, GrEngine::Entity* parent, bool read_only)
{
	property_name = name;
	property_type = PropertyType::Custom;
	property_value = value;
	owner = parent;
	locked = read_only;
}

StringProperty::StringProperty(PropertyType type, std::string value, GrEngine::Entity* parent, bool read_only)
{
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	property_value = value;
	owner = parent;
	locked = read_only;
}

StringProperty::~StringProperty()
{

}

const char* StringProperty::ValueString()
{
	return property_value.c_str();
}

void StringProperty::ParsePropertyValue(const char* value)
{
	if (locked) return;

	SetPropertyValue(std::string(value));
}

void StringProperty::SetPropertyValue(std::string value)
{
	if (locked) return;

	property_value = value;

	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity* )owner, this);
	}
}

std::any StringProperty::GetAnyValue()
{
	return property_value;
}

void* StringProperty::GetValueAdress()
{
	return &property_value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FloatProperty::FloatProperty(const char* name, float value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = name;
	property_type = PropertyType::Custom;
	string_value = std::to_string(value);
	owner = parent;
	locked = read_only;
}

FloatProperty::FloatProperty(PropertyType type, float value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	string_value = std::to_string(value);
	owner = parent;
	locked = read_only;
}

FloatProperty::~FloatProperty()
{

}

const char* FloatProperty::ValueString()
{
	string_value = GrEngine::Globals::FloatToString(property_value, 5);
	return string_value.c_str();
}

void FloatProperty::ParsePropertyValue(const char* value)
{
	if (locked) return;

	SetPropertyValue(std::stof(value));
}

void FloatProperty::SetPropertyValue(float value)
{
	if (locked) return;

	property_value = value;

	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity*)owner, this);
	}
}

std::any FloatProperty::GetAnyValue()
{
	return property_value;
}

void* FloatProperty::GetValueAdress()
{
	return &property_value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IntegerProperty::IntegerProperty(const char* name, int value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = name;
	property_type = PropertyType::Custom;
	string_value = std::to_string(value);
	owner = parent;
	locked = read_only;
}

IntegerProperty::IntegerProperty(PropertyType type, int value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	string_value = std::to_string(value);
	owner = parent;
	locked = read_only;
}

IntegerProperty::~IntegerProperty()
{

}

const char* IntegerProperty::ValueString()
{
	string_value = std::to_string(property_value);
	return string_value.c_str();
}

void IntegerProperty::ParsePropertyValue(const char* value)
{
	if (locked) return;

	SetPropertyValue(std::stoi(value));
}

void IntegerProperty::SetPropertyValue(int value)
{
	if (locked) return;

	property_value = value;

	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity*)owner, this);
	}
}

std::any IntegerProperty::GetAnyValue()
{
	return property_value;
}

void* IntegerProperty::GetValueAdress()
{
	return &property_value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Vector3fProperty::Vector3fProperty(const char* name, glm::vec3 value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = name;
	property_type = EntityProperty::StringToType(name);
	owner = parent;
	locked = read_only;
}

Vector3fProperty::Vector3fProperty(const char* name, float x, float y, float z, GrEngine::Entity* parent, bool read_only)
{
	property_value = { x, y, z };
	property_name = name;
	property_type = EntityProperty::StringToType(name);
	owner = parent;
	locked = read_only;
}

Vector3fProperty::Vector3fProperty(PropertyType type, glm::vec3 value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	owner = parent;
	locked = read_only;
}

Vector3fProperty::Vector3fProperty(PropertyType type, float x, float y, float z, GrEngine::Entity* parent, bool read_only)
{
	property_value = { x, y, z };
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	owner = parent;
	locked = read_only;
}

Vector3fProperty::~Vector3fProperty()
{

}

const char* Vector3fProperty::ValueString()
{
	string_value = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5));
	return string_value.c_str();
}

void Vector3fProperty::ParsePropertyValue(const char* value)
{
	if (locked) return;

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

	SetPropertyValue(res);
}

void Vector3fProperty::SetPropertyValue(const float& x, const float& y, const float& z)
{
	if (locked) return;

	SetPropertyValue({ x, y, z });
}

void Vector3fProperty::SetPropertyValue(glm::vec3 value)
{
	if (locked) return;

	property_value = value;

	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity*)owner, this);
	}
}

std::any Vector3fProperty::GetAnyValue()
{
	return property_value;
}

void* Vector3fProperty::GetValueAdress()
{
	return &property_value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Vector4fProperty::Vector4fProperty(const char* name, glm::vec4 value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = name;
	property_type = EntityProperty::StringToType(name);
	owner = parent;
	locked = read_only;
}

Vector4fProperty::Vector4fProperty(const char* name, float x, float y, float z, float w, GrEngine::Entity* parent, bool read_only)
{
	property_value = { x, y, z, w };
	property_name = name;
	property_type = EntityProperty::StringToType(name);
	owner = parent;
	locked = read_only;
}

Vector4fProperty::Vector4fProperty(PropertyType type, glm::vec4 value, GrEngine::Entity* parent, bool read_only)
{
	property_value = value;
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	owner = parent;
	locked = read_only;
}

Vector4fProperty::Vector4fProperty(PropertyType type, float x, float y, float z, float w, GrEngine::Entity* parent, bool read_only)
{
	property_value = { x, y, z, w };
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	owner = parent;
	locked = read_only;
}

Vector4fProperty::~Vector4fProperty()
{

}

const char* Vector4fProperty::ValueString()
{
	string_value = (GrEngine::Globals::FloatToString(property_value.x, 5) + ":" + GrEngine::Globals::FloatToString(property_value.y, 5) + ":" + GrEngine::Globals::FloatToString(property_value.z, 5) + ":" + GrEngine::Globals::FloatToString(property_value.w, 5));
	return string_value.c_str();
}

void Vector4fProperty::ParsePropertyValue(const char* value)
{
	if (locked) return;

	auto cols = GrEngine::Globals::SeparateString(value, ':');
	if (cols.size() < 4) return;
	glm::vec4 res{};

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

	SetPropertyValue(res);
}

void Vector4fProperty::SetPropertyValue(const float& x, const float& y, const float& z, const float& w)
{
	if (locked) return;

	SetPropertyValue({ x, y, z, w });
}

void Vector4fProperty::SetPropertyValue(glm::vec4 value)
{
	if (locked) return;

	property_value = value;

	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity*)owner, this);
	}
}

std::any Vector4fProperty::GetAnyValue()
{
	return property_value;
}

void* Vector4fProperty::GetValueAdress()
{
	return &property_value;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PointerProperty::PointerProperty(const char* name, void* value, DestructionCallback function, GrEngine::Entity* parent)
{
	property_value = value;
	property_name = name;
	property_type = PropertyType::Custom;
	destruct = function;
	owner = parent;
	locked = true;
}

PointerProperty::PointerProperty(PropertyType type, void* value, DestructionCallback function, GrEngine::Entity* parent)
{
	property_value = value;
	property_name = EntityProperty::TypeToString(type);
	property_type = type;
	destruct = function;
	owner = parent;
	locked = true;
}

PointerProperty::~PointerProperty()
{
	destruct.value()(owner, this);
}

const char* PointerProperty::ValueString()
{
	return "nil";
}

void PointerProperty::ParsePropertyValue(const char* value)
{
	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity*)owner, this);
	}
}

void PointerProperty::SetPropertyValue(void* value)
{
	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity*)owner, this);
	}
}

std::any PointerProperty::GetAnyValue()
{
	return property_value;
}

void* PointerProperty::GetValueAdress()
{
	return property_value;
}





////////////////////////////////////EntityOrientation/////////////////////////////////////////////

EntityOrientationProperty::EntityOrientationProperty(const float& pitch, const float& yaw, const float& roll, GrEngine::Entity* parent)
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

	if (callback.has_value())
	{
		callback.value()((GrEngine::Entity*)owner, this);
	}
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
	//Get approximation of PYR degree values
	glm::mat4 q = glm::mat4_cast(value);
	pitch_yaw_roll = glm::degrees(glm::vec3(glm::eulerAngles(glm::quat_cast(glm::inverse(q)))));
	pitch_yaw_roll *= -1.f;
}

glm::vec3 EntityOrientationProperty::GetPitchYawRoll()
{
	return pitch_yaw_roll;
}


std::any EntityOrientationProperty::GetAnyValue()
{
	return property_value;
}

void* EntityOrientationProperty::GetValueAdress()
{
	return &property_value;
}

////////////////////////////////////Cubemap Property/////////////////////////////////////////////

CubemapProperty::CubemapProperty(std::array<std::string, 6> textures, GrEngine::Entity* parent)
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
		static_cast<GrEngine::Skybox*>(owner)->AssignTextures(property_value);
}

void CubemapProperty::SetPropertyValue(std::array<std::string, 6> value)
{
	property_value = value;

	if (owner != nullptr)
		static_cast<GrEngine::Skybox*>(owner)->AssignTextures(property_value);
}

std::any CubemapProperty::GetAnyValue()
{
	return property_value;
}

void* CubemapProperty::GetValueAdress()
{
	return &property_value;
}