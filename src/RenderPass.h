#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <Shader.h>
#include <vector>

class RenderPass
{
public:
	RenderPass() {};


	void setShader(Shader& shader)
	{
		m_Shader = shader;
	}

	void bindData(bool finalPass)
	{
		if(!finalPass)
			glGenFramebuffers(1, &FBO);
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);

		std::vector<glm::vec3> square = { glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), glm::vec3(-1, 1, 0), 
										  glm::vec3(1, 1, 0), glm::vec3(-1, 1, 0), glm::vec3(1, -1, 0) };
		
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		
		unsigned int VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, square.size() * sizeof(glm::vec3), &square[0], GL_STATIC_DRAW);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

		if (!finalPass)
		{
			std::vector<GLuint> attachments;
			for (int i = 0; i < colorAttachment.size(); ++i)
			{
				glBindTexture(GL_TEXTURE_2D, colorAttachment[0]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorAttachment[i], 0);
				attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
			}
			glDrawBuffers(attachments.size(), &attachments[0]);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

	}
	

	void draw(std::vector<GLuint> texPassArray = {})
	{
		m_Shader.use();
		glBindFramebuffer(GL_FRAMEBUFFER, FBO);
		glBindVertexArray(VAO);
		for (int i = 0; i < texPassArray.size(); ++i)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, texPassArray[i]);
			std::string texName = "texPass" + std::to_string(i);
			m_Shader.setInt(texName, i);
		}
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glBindVertexArray(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

public:
	std::vector<GLuint> colorAttachment;
	Shader m_Shader;

private:
	unsigned int FBO, VAO;
	int width = 1280, height = 640;
};

