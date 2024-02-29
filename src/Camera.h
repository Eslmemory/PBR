#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
	Camera(unsigned int width, unsigned int height, unsigned int level)
		: m_AspectRatio(float(width / height)), m_Level(float(level))
	{	
	}

	void SetTranslation(glm::vec3 translation) { m_translation = glm::translate(m_translation, translation); }
	void SetRotation(float degree, glm::vec3 axis) { m_rotation = glm::rotate(m_rotation, glm::radians(degree), axis); }

	void CalculateViewMatrix() { m_ViewMatrix = glm::inverse(m_rotation * m_translation); }
	void CalculateProjectionMatrix() { m_ProjectionMatrix =  glm::ortho(-m_AspectRatio, m_AspectRatio, -1.0f, 1.0f); }

	glm::mat4 GetViewMatrix() { CalculateViewMatrix(); return m_ViewMatrix; }
	glm::mat4 GetProjectionMatrix() { CalculateProjectionMatrix(); return m_ProjectionMatrix; }

	glm::mat4 GetViewProjectionMatrix() { return m_ProjectionMatrix * m_ViewMatrix; }

private:
	float m_AspectRatio;
	float m_Level;
	glm::mat4 m_translation;
	glm::mat4 m_rotation;
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjectionMatrix;
};
