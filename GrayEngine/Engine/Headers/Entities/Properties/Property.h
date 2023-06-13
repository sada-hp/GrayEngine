#pragma once
#include "pch.h"
#include "Engine/Headers/Core/Globals.h"

namespace GrEngine
{
	class Entity;
};

enum class PropertyType
{
	Error = -1,
	EntityID = 100,
	Mass = 101,
	EntityName,
	Scale,
	EntityPosition,
	EntityOrientation,
	Color,
	PhysComponent,
	PlayerController,
	BodyType,
	Drawable,
	Spotlight,
	CascadeLight,
	PointLight,
	OmniLight,
	Cubemap,
	Shader,
	Transparency,
	DoubleSided,
	CastShadow,
	CollisionType,
	ModelPath,
	AlphaThreshold,
	MaximumDistance,
	Brightness,
	NormalStrength,
	Custom
};

struct EntityProperty
{
public:
	typedef void (*PropertyChangedCallback)(GrEngine::Entity* owner, EntityProperty* self);
	typedef void (*DestructionCallback)(GrEngine::Entity* owner, EntityProperty* self);

	EntityProperty() {};
	virtual ~EntityProperty() { owner = nullptr; property_name = nullptr; };
	virtual const char* ValueString() = 0;
	virtual std::any GetAnyValue() = 0;
	virtual void ParsePropertyValue(const char* value) = 0;
	virtual void* GetValueAdress() = 0;
	const char* PropertyNameString()
	{
		return property_name;
	}
	const PropertyType& GetPropertyType()
	{
		return property_type;
	}

	static PropertyType StringToType(const char* property_name)
	{
		const static std::unordered_map<std::string, PropertyType> type_hash =
		{ {"EntityID", PropertyType::EntityID},
		{"Mass", PropertyType::Mass},
		{"EntityName", PropertyType::EntityName},
		{"Scale", PropertyType::Scale},
		{"EntityPosition", PropertyType::EntityPosition},
		{"EntityOrientation", PropertyType::EntityOrientation},
		{"Color", PropertyType::Color},
		{"Physics", PropertyType::PhysComponent},
		{"PhysComponent", PropertyType::PhysComponent},
		{"PlayerController", PropertyType::PlayerController},
		{"MovementComponent", PropertyType::PlayerController},
		{"BodyType", PropertyType::BodyType},
		{"Drawable", PropertyType::Drawable},
		{"Mesh", PropertyType::Drawable},
		{"Spotlight", PropertyType::Spotlight},
		{"Cascade", PropertyType::CascadeLight},
		{"CascadeLight", PropertyType::CascadeLight},
		{"PointLight", PropertyType::PointLight},
		{"Pointlight", PropertyType::PointLight},
		{"OmniLight", PropertyType::OmniLight},
		{"Omni", PropertyType::OmniLight},
		{"CubemapProperty", PropertyType::Cubemap},
		{"Cubemap", PropertyType::Cubemap},
		{"Shader", PropertyType::Shader},
		{"Transparency", PropertyType::Transparency},
		{"DoubleSided", PropertyType::DoubleSided},
		{"CastShadow", PropertyType::CastShadow},
		{"CollisionType", PropertyType::CollisionType},
		{"ModelPath", PropertyType::ModelPath},
		{"AlphaThreshold", PropertyType::AlphaThreshold},
		{"MaximumDistance", PropertyType::MaximumDistance},
		{"Brightness", PropertyType::Brightness},
		{"NormalStrength", PropertyType::NormalStrength}
		};

		auto it = type_hash.find(std::string(property_name));
		if (it != type_hash.end())
			return it->second;
		else
			throw "Unknow type provided!";
	}

	static const char* TypeToString(PropertyType property_type)
	{
		switch (property_type)
		{
		case PropertyType::Error:
			return "nil";
		case PropertyType::EntityID:
			return "EntityID";
		case PropertyType::Mass:
			return "Mass";
		case PropertyType::EntityName:
			return "EntityName";
		case PropertyType::Scale:
			return "Scale";
		case PropertyType::EntityPosition:
			return "EntityPosition";
		case PropertyType::EntityOrientation:
			return "EntityOrientation";
		case PropertyType::Color:
			return "Color";
		case PropertyType::PhysComponent:
			return "PhysComponent";
		case PropertyType::PlayerController:
			return "PlayerController";
		case PropertyType::Drawable:
			return "Drawable";
		case PropertyType::Spotlight:
			return "Spotlight";
		case PropertyType::CascadeLight:
			return "CascadeLight";
		case PropertyType::PointLight:
			return "PointLight";
		case PropertyType::OmniLight:
			return "OmniLight";
		case PropertyType::Cubemap:
			return "CubemapProperty";
		case PropertyType::Shader:
			return "Shader";
		case PropertyType::Transparency:
			return "Transparency";
		case PropertyType::DoubleSided:
			return "DoubleSided";
		case PropertyType::CastShadow:
			return "CastShadow";
		case PropertyType::CollisionType:
			return "CollisionType";
		case PropertyType::BodyType:
			return "BodyType";
		case PropertyType::ModelPath:
			return "ModelPath";
		case PropertyType::AlphaThreshold:
			return "AlphaThreshold";
		case PropertyType::MaximumDistance:
			return "MaximumDistance";
		case PropertyType::Brightness:
			return "Brightness";
		case PropertyType::NormalStrength:
			return "NormalStrength";
		default:
			return "Custom";
		}
	}

	void SetCallback(PropertyChangedCallback function)
	{
		callback = function;
	}
protected:
	const char* property_name;
	PropertyType property_type;
	GrEngine::Entity* owner;
	bool locked = false;
	std::optional<PropertyChangedCallback> callback;
};

struct StringProperty : public EntityProperty
{
public:
	StringProperty(const char* name, std::string value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	StringProperty(PropertyType type, std::string value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	~StringProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

private:
	std::string property_value;
};

struct FloatProperty : public EntityProperty
{
public:
	FloatProperty(const char* name, float value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	FloatProperty(PropertyType type, float value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	~FloatProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(float value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

private:
	std::string string_value;
	float property_value;
};

struct IntegerProperty : public EntityProperty
{
public:
	IntegerProperty(const char* name, int value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	IntegerProperty(PropertyType type, int value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	~IntegerProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(int value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

private:
	std::string string_value;
	int property_value;
};

struct Vector3fProperty : public EntityProperty
{
public:
	Vector3fProperty(const char* name, glm::vec3 value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	Vector3fProperty(PropertyType type, glm::vec3 value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	Vector3fProperty(const char* name, float x, float y, float z, GrEngine::Entity* parent = nullptr, bool read_only = false);
	Vector3fProperty(PropertyType type, float x, float y, float z, GrEngine::Entity* parent = nullptr, bool read_only = false);
	~Vector3fProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(glm::vec3 value);
	void SetPropertyValue(const float& x, const float& y, const float& z);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

private:
	std::string string_value;
	glm::vec3 property_value;
};

struct Vector4fProperty : public EntityProperty
{
public:
	Vector4fProperty(const char* name, glm::vec4 value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	Vector4fProperty(PropertyType type, glm::vec4 value, GrEngine::Entity* parent = nullptr, bool read_only = false);
	Vector4fProperty(const char* name, float x, float y, float z, float w, GrEngine::Entity* parent = nullptr, bool read_only = false);
	Vector4fProperty(PropertyType type, float x, float y, float z, float w, GrEngine::Entity* parent = nullptr, bool read_only = false);
	~Vector4fProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(glm::vec4 value);
	void SetPropertyValue(const float& x, const float& y, const float& z, const float& w);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

private:
	std::string string_value;
	glm::vec4 property_value;
};

struct PointerProperty : public EntityProperty
{
public:
	PointerProperty(const char* name, void* value, DestructionCallback function, GrEngine::Entity* parent = nullptr);
	PointerProperty(PropertyType type, void* value, DestructionCallback function, GrEngine::Entity* parent = nullptr);
	~PointerProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(void* value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

private:
	void* property_value;
	std::optional<DestructionCallback> destruct;
};



struct EntityOrientationProperty : public EntityProperty
{
public:
	EntityOrientationProperty(const float& pitch, const float& yaw, const float& roll, GrEngine::Entity* parent = nullptr);
	~EntityOrientationProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(glm::vec3 p_y_r);
	void SetPropertyValue(glm::quat value);
	glm::vec3 GetPitchYawRoll();
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	glm::quat property_value;
private:
	std::string property_string;
	glm::vec3 pitch_yaw_roll = { 0.f, 0.f, 0.f };
};

struct CubemapProperty : public EntityProperty
{
public:
	CubemapProperty(std::array<std::string,6> textures, GrEngine::Entity* parent = nullptr);
	~CubemapProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::array<std::string, 6> value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::array<std::string, 6> property_value;
private:
	std::string property_string;
};