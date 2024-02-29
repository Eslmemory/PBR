#pragma once

#include <string>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

class Texture
{
public:
	Texture() = default;
	Texture(const std::string& imageFile)
	{
		stbi_set_flip_vertically_on_load(true);
		glGenTextures(1, &m_TextureID);
		unsigned char* data = stbi_load(imageFile.c_str(), &m_Width, &m_Height, &m_Channel, 0);
		if (data)
		{
			if (m_Channel == 1)
				m_Format = GL_RED;
			else if (m_Channel == 3)
				m_Format = GL_RGB;
			else if (m_Channel == 4)
				m_Format = GL_RGBA;

			glBindTexture(GL_TEXTURE_2D, m_TextureID);
			glTexImage2D(GL_TEXTURE_2D, 0, m_Format, m_Width, m_Height, 0, m_Format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else
		{
			std::cout << "Texture failed to load : " << imageFile << std::endl;
		}
	}

	void bind(unsigned int slot)
	{
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, m_TextureID);
	}

private:
	unsigned int m_TextureID;
	int m_Width, m_Height, m_Channel;
	GLenum m_Format;
};
