#pragma once
#include "pch.h"
#include "Engine/Headers/Core/Globals.h"

enum class PropertyType
{
	EntityID = 100,
	Mass = 101,
	EntityName,
	Scale,
	EntityPosition,
	EntityOrientation,
	Color,
	Drawable,
	Cubemap,
	Shader,
	Transparency
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
	PropertyType& GetPropertyType()
	{
		return property_type;
	}

	const char* property_name;
	PropertyType property_type;
	void* property_value;
	void* owner;
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
	void SetPropertyValue(const float& pitch, const float& yaw, const float& roll);
	void SetPropertyValue(glm::quat value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	glm::quat property_value;
private:
	std::string property_string;
	glm::vec3 pitch_yaw_roll = { 0.f, 0.f, 0.f };
	glm::vec3 degrees = { 0.f, 0.f, 0.f };
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

	std::string property_value;
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
	Transparency(int value, void* parent = nullptr);
	~Transparency();
	const char* ValueString() override;
	void ParsePropertyValue(const char* value) override;
	void SetPropertyValue(int value);
	std::any GetAnyValue() override;
	virtual void* GetValueAdress() override;

	int property_value;
private:
	std::string property_string;
};