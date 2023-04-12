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
	Drawable,
	Spotlight,
	Cubemap,
	Shader,
	Transparency,
	DoubleSided,
	CastShadow
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
		{"Drawable", PropertyType::Drawable},
		{"Mesh", PropertyType::Drawable},
		{"Spotlight", PropertyType::Spotlight},
		{"CubemapProperty", PropertyType::Cubemap},
		{"Cubemap", PropertyType::Cubemap},
		{"Shader", PropertyType::Shader},
		{"Transparency", PropertyType::Transparency},
		{"DoubleSided", PropertyType::DoubleSided},
		{"CastShadow", PropertyType::CastShadow}
		};

		auto it = type_hash.find(std::string(property_name));
		if (it != type_hash.end())
			return it->second;
		else
			throw "Unknow type provided!";
	}
};

struct EntityID : public EntityProperty
{
public:
	EntityID(UINT id, void* parent = nullptr);
	~EntityID();
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

struct Mass : public EntityProperty
{
public:
	Mass(float mass, void* parent = nullptr);
	~Mass();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(float value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	float property_value;
private:
	std::string string_value;
};

struct EntityName : public EntityProperty
{
public:
	EntityName(const char* name, void* parent = nullptr);
	~EntityName();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(const char* value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value;
};

struct Scale : public EntityProperty
{
public:
	Scale(float x, float y, float z, void* parent = nullptr);
	~Scale();
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

struct EntityPosition : public EntityProperty
{
public:
	EntityPosition(float x, float y, float z, void* parent = nullptr);
	~EntityPosition();
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

struct EntityOrientation : public EntityProperty
{
public:
	EntityOrientation(const float& pitch, const float& yaw, const float& roll, void* parent = nullptr);
	~EntityOrientation();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(glm::vec3 p_y_r);
	void SetPropertyValue(glm::quat value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	glm::quat property_value;
private:
	std::string property_string;
	glm::vec3 pitch_yaw_roll = { 0.f, 0.f, 0.f };
};

struct Color : public EntityProperty
{
public:
	Color(void* parent = nullptr);
	Color(const float& r, const float& g, const float& b, const float& a, void* parent = nullptr);
	Color(const float& r, const float& g, const float& b, void* parent = nullptr);
	~Color();
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

struct Drawable : public EntityProperty
{
public:
	Drawable(const char* path, void* parent = nullptr);
	~Drawable();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value = "nil";
	void* drawable = nullptr;
};

struct SpotLight : public EntityProperty
{
public:
	SpotLight(void* parent = nullptr);
	~SpotLight();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value = "nil";
	void* spotlight = nullptr;
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

struct Shader : public EntityProperty
{
public:
	Shader(const char* path, void* parent = nullptr);
	~Shader();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(std::string value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	std::string property_value;
};

struct Transparency : public EntityProperty
{
public:
	Transparency(bool value, void* parent = nullptr);
	~Transparency();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(bool value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value;
private:
	std::string property_string;
};

struct DoubleSided : public EntityProperty
{
public:
	DoubleSided(bool value, void* parent = nullptr);
	~DoubleSided();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(bool value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value;
private:
	std::string property_string;
};

struct CastShadow : public EntityProperty
{
public:
	CastShadow(bool value, void* parent = nullptr);
	~CastShadow();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(int value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value;
private:
	std::string property_string;
};