#pragma once
#include <GrayEngine.h>

namespace GrEngine
{
	class Character : public Entity
	{
	public:
		Character()
		{
            controller = static_cast<MovementComponent*>(AddNewProperty(PropertyType::PlayerController)->GetValueAdress());
            phys_comp = static_cast<PhysicsObject*>(GetProperty(PropertyType::PhysComponent)->GetValueAdress());
            phys_comp->GenerateCapsuleCollision(1, 2.8f);
		}

		Character(UINT id) : Entity(id)
		{
            controller = static_cast<MovementComponent*>(AddNewProperty(PropertyType::PlayerController)->GetValueAdress());
            phys_comp = static_cast<PhysicsObject*>(GetProperty(PropertyType::PhysComponent)->GetValueAdress());
            phys_comp->GenerateCapsuleCollision(1, char_height);
		}

		~Character()
		{
			
		}

		PhysicsObject* GetPhysicsComponent()
		{
			return phys_comp;
		}

		void WalkInDirection(glm::vec3 direction)
		{
            if (direction != glm::vec3(0.f) && controller->IsGrounded())
            {
                landed = true;
                last_dir = glm::normalize(direction);
                actual_speed = walk_speed;
                last_speed = actual_speed;
                float speed = actual_speed * Globals::delta_time;
                controller->MoveObject(glm::vec3(speed, 0.f, speed) * last_dir);
            }
            else if (!controller->IsGrounded() && !landed)
            {
                last_speed = last_speed * 0.995f;
                if (direction != glm::vec3(0.f))
                    last_dir = (15.f * last_dir + glm::normalize(direction))/16.f;

                float speed = last_speed * Globals::delta_time;
                controller->MoveObject(glm::vec3(speed, 0.f, speed) * last_dir);
            }
            else
            {
                std::vector<GrEngine::RayCastResult> res = phys_comp->GetSimulationContext()->GetObjectContactPoints(phys_comp, 15.f);

                actual_speed = actual_speed * 0.8f;
                if (actual_speed < 0.01f)
                {
                    last_dir = glm::vec3(0.f, 0.f, 0.f);
                    actual_speed = 0.f;
                }
                last_speed = actual_speed;
                float speed = actual_speed * Globals::delta_time;

                if (res.size() > 0)
                {
                    landed = true;
                    float ang = 0.f;
                    constexpr float max_ang = glm::radians(45.f);
                    glm::vec3 normal = glm::vec3(0.f);

                    for (int i = 0; i < res.size(); i++)
                    {
                        if (res[i].id != *obj_id)
                        {
                            normal += res[i].hitNorm;
                        }
                    }
                    
                    normal = normal / (float)res.size();
                    normal = glm::normalize(normal);
                    ang = glm::acos(glm::abs(glm::dot(glm::vec3(0, 1, 0), normal)));
                    bool sliding = ang > max_ang;
                    if (sliding && normal != glm::vec3(0.f))
                    {
                        constexpr float limit = glm::radians(20.f);
                        float bias = 0.15f * (ang/(max_ang + limit));
                        speed = 0.025f + bias;
                        speed = glm::abs(speed);
                        glm::vec3 slide_dir = glm::cross(-glm::cross(normal, glm::vec3(0, 1, 0)), normal);
                        last_dir = slide_dir;
                        controller->MoveObject(glm::vec3(speed, 0.f, speed) * slide_dir);
                        //Logger::Out("angle is %f", OutputType::Log, glm::degrees(ang));
                    }
                    else
                    {
                        controller->MoveObject(glm::vec3(speed, 0.f, speed) * last_dir);
                    }
                }
                else if (!controller->IsGrounded())
                {
                    controller->MoveObject(glm::vec3(last_speed * Globals::delta_time, 0.f, last_speed * Globals::delta_time) * last_dir);
                    landed = false;
                }
                else
                {
                    last_speed = 0.f;
                    last_dir = glm::vec3(0.f, 0.f, 0.f);
                    controller->MoveObject(glm::vec3(0.f, 0.f, 0.f));
                }
            }
		}

        void SetWalkingSpeed(float speed)
        {
            walk_speed = speed;
        }

        void Jump()
        {
            controller->Jump();
            landed = false;
        }
	private:
        float walk_speed = 10.f;
        float actual_speed = 10.f;
        float last_speed = 10.f;
        PhysicsObject* phys_comp;
        MovementComponent* controller;
        glm::vec3 last_dir;
        bool landed = true;
        float step_height = 0.25f;
        float char_height = 2.8f;
	};
}