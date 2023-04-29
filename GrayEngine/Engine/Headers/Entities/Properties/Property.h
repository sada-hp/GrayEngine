#pragma once
#include "pch.h"
#include "Engine/Headers/Core/Globals.h"

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
	CollisionType
};

struct EntityProperty
{
public:
	EntityProperty() {};
	virtual ~EntityProperty() { owner = nullptr; property_name = nullptr; };
	virtual const char* ValueString() = 0;
	virtual std::any GetAnyValue() = 0;
	virtual void ParsePropertyValue(const char* value) = 0;
	virtual void* GetValueAdress() = 0;
	const char* PrpertyNameString()
	{
		return property_name;
	}
	const PropertyType& GetPropertyType()
	{
		return property_type;
	}

	const char* property_name;
	PropertyType property_type;
	void* property_value;
	void* owner;

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
		{"CollisionType", PropertyType::CollisionType}
		};

		auto it = type_hash.find(std::string(property_name));
		if (it != type_hash.end())
			return it->second;
		else
			throw "Unknow type provided!";
	}
};

struct EntityIDProperty : public EntityProperty
{
public:
	EntityIDProperty(UINT id, void* parent = nullptr);
	~EntityIDProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(UINT value);
	UINT GetValue();
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	UINT property_value;
private:
	std::string string_value;
};

struct MassProperty : public EntityProperty
{
public:
	MassProperty(float mass, void* parent = nullptr);
	~MassProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(float value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	float property_value;
private:
	std::string string_value;
};

struct EntityNameProperty : public EntityProperty
{
public:
	EntityNameProperty(const char* name, void* parent = nullptr);
	~EntityNameProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(const char* value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value;
};

struct ScaleProperty : public EntityProperty
{
public:
	ScaleProperty(float x, float y, float z, void* parent = nullptr);
	~ScaleProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(const float& x, const float& y, const float& z);
	void SetPropertyValue(const glm::vec3& value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	glm::vec3 property_value;
private:
	std::string property_string;
};

struct EntityPositionProperty : public EntityProperty
{
public:
	EntityPositionProperty(float x, float y, float z, void* parent = nullptr);
	~EntityPositionProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(const float& x, const float& y, const float& z);
	void SetPropertyValue(const glm::vec3& value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	glm::vec3 property_value;
private:
	std::string property_string;
};

struct EntityOrientationProperty : public EntityProperty
{
public:
	EntityOrientationProperty(const float& pitch, const float& yaw, const float& roll, void* parent = nullptr);
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

struct ColorProperty : public EntityProperty
{
public:
	ColorProperty(void* parent = nullptr);
	ColorProperty(const float& r, const float& g, const float& b, const float& a, void* parent = nullptr);
	ColorProperty(const float& r, const float& g, const float& b, void* parent = nullptr);
	~ColorProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(const float& r, const float& g, const float& b, const float& a);
	void SetPropertyValue(glm::vec4 value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	glm::vec4 property_value;
private:
	std::string property_string;
};

struct PhysComponentProperty : public EntityProperty
{
public:
	PhysComponentProperty(void* parent = nullptr);
	~PhysComponentProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(int value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	void* phys = nullptr;
	int property_value = 0;
private:
	std::string property_string = "0";
};

struct CollisionTypeProperty : public EntityProperty
{
public:
	CollisionTypeProperty(void* parent = nullptr);
	~CollisionTypeProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(int value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value = 0;
private:
	std::string property_string = "0";
};

struct DrawableProperty : public EntityProperty
{
public:
	DrawableProperty(const char* path, void* parent = nullptr);
	~DrawableProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value = "nil";
	void* drawable = nullptr;
};

struct SpotLightProperty : public EntityProperty
{
public:
	SpotLightProperty(void* parent = nullptr);
	~SpotLightProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value = "nil";
	void* spotlight = nullptr;
};

struct CascadeProperty : public EntityProperty
{
public:
	CascadeProperty(void* parent = nullptr);
	~CascadeProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value = "nil";
	void* cascade = nullptr;
};

struct PointLightPropery : public EntityProperty
{
public:
	PointLightPropery(void* parent = nullptr);
	~PointLightPropery();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value = "nil";
	void* point = nullptr;
};

struct OmniLightPropery : public EntityProperty
{
public:
	OmniLightPropery(void* parent = nullptr);
	~OmniLightPropery();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value = "nil";
	void* omni = nullptr;
};


struct CubemapProperty : public EntityProperty
{
public:
	CubemapProperty(std::array<std::string,6> textures, void* parent = nullptr);
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

struct ShaderProperty : public EntityProperty
{
public:
	ShaderProperty(const char* path, void* parent = nullptr);
	~ShaderProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value;
};

struct TransparencyProperty : public EntityProperty
{
public:
	TransparencyProperty(bool value, void* parent = nullptr);
	~TransparencyProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(bool value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value;
private:
	std::string property_string;
};

struct DoubleSidedProperty : public EntityProperty
{
public:
	DoubleSidedProperty(bool value, void* parent = nullptr);
	~DoubleSidedProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(bool value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value;
private:
	std::string property_string;
};

struct CastShadowProperty : public EntityProperty
{
public:
	CastShadowProperty(bool value, void* parent = nullptr);
	~CastShadowProperty();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(int value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value;
private:
	std::string property_string;
};