#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <iostream>
#include <sstream>

class Shader
{
public:
	Shader() {}
	Shader(const char* vertexPath, const char* fragPath)
	{
		std::string vertexCode, fragCode;
		std::ifstream vertexFile, fragFile;
		std::stringstream vertexStream, fragStream;

		vertexFile.open(vertexPath);
		fragFile.open(fragPath);
		vertexStream << vertexFile.rdbuf();
		fragStream << fragFile.rdbuf();
		vertexFile.close();
		fragFile.close();

		vertexCode = vertexStream.str();
		fragCode = fragStream.str();

		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragCode.c_str();

		unsigned int vertex, fragment;
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		CheckCompileError(vertex, "VERTEX");

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		CheckCompileError(fragment, "FRAGMENT");
		
		programID = glCreateProgram();
		glAttachShader(programID, vertex);
		glAttachShader(programID, fragment);

		glLinkProgram(programID);
		CheckCompileError(programID, "PROGRAM");

		glDeleteShader(vertex);
		glDeleteShader(fragment);
	}

	void CheckCompileError(unsigned int shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR:SHADER_COMPLIE_ERROR of type: " << type << std::endl << infoLog << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR:PROGRAM_LINK_ERROR: " << std::endl << infoLog << std::endl;
			}
		}
	}

	void use()
	{
		glUseProgram(programID);
	}

	void setMat4(const std::string& name, glm::mat4 matrix)
	{
		GLint location = glGetUniformLocation(programID, name.c_str());
		glUniformMatrix4fv(location, 1, false, &matrix[0][0]);
	}

	void setInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(programID, name.c_str());
		glUniform1i(location, value);
	}

	void setFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(programID, name.c_str());
		glUniform1f(location, value);
	}

	void setVec2(const std::string& name, glm::vec2 value)
	{
		GLint location = glGetUniformLocation(programID, name.c_str());
		glUniform2f(location, value[0], value[1]);
	}

	void setVec3(const std::string& name, glm::vec3 value)
	{
		GLint location = glGetUniformLocation(programID, name.c_str());
		glUniform3f(location, value[0], value[1], value[2]);
	}

private:
	uint32_t programID;
};
