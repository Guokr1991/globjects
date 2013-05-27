#include <glow/Program.h>
#include <glow/Log.h>

#include <vector>
#include <glm/gtc/type_ptr.hpp>

using namespace glow;

Program::Program()
: Object(createProgram())
, _linked(false)
, _dirty(true)
{
}

Program::~Program()
{
	for (ref_ptr<Shader> shader: std::set<ref_ptr<Shader>>(_shaders))
	{
		detach(shader);
	}
	for (std::pair<std::string, ref_ptr<Uniform>> uniformPair: _uniforms)
	{
		uniformPair.second->removeFrom(this);
	}
	if (_id) glDeleteProgram(_id);
}

GLuint Program::createProgram()
{
	return glCreateProgram();
}

void Program::use()
{
	checkDirty();
	glUseProgram(_id);
}

void Program::release()
{
	glUseProgram(0);
}

bool Program::isUsed() const
{
	GLint currentProgram = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

	return currentProgram>0 && currentProgram == _id;
}

bool Program::isLinked() const
{
	return _linked;
}

void Program::invalidate()
{
	_dirty = true;
}

void Program::checkDirty()
{
	if (_dirty)
	{
		link();
	}
}

void Program::attach(Shader* shader)
{
	glAttachShader(_id, shader->id());

	shader->addToProgram(this);
	_shaders.insert(shader);

	invalidate();
}

void Program::detach(Shader* shader)
{
	glDetachShader(_id, shader->id());

	shader->removeFromProgram(this);
	_shaders.erase(shader);

	invalidate();
}

void Program::link()
{
	glLinkProgram(_id);
	checkLinkStatus();
	_dirty = false;
	updateUniforms();
}

void Program::bindFragDataLocation(GLuint index, const std::string& name)
{
	glBindFragDataLocation(_id, index, name.c_str());
}

void Program::bindAttributeLocation(GLuint index, const std::string& name)
{
	glBindAttribLocation(_id, index, name.c_str());
}

GLint Program::getUniformLocation(const std::string& name)
{
	checkDirty();
	return glGetUniformLocation(_id, name.c_str());
}

GLint Program::getAttributeLocation(const std::string& name)
{
	checkDirty();
	return glGetAttribLocation(_id, name.c_str());
}

GLuint Program::getResourceIndex(GLenum programInterface, const std::string& name)
{
	checkDirty();
	return glGetProgramResourceIndex(_id, programInterface, name.c_str());
}

void Program::addUniform(Uniform* uniform)
{
	_uniforms[uniform->name()] = uniform;
	uniform->addTo(this);
	if (_linked) uniform->updateFor(this);
}

Uniform* Program::getUniform(const std::string& name)
{
	if (!_uniforms[name])
	{
		Uniform* uniform = new Uniform(name);
		_uniforms[uniform->name()] = uniform;
		uniform->addTo(this);
	}
	return _uniforms[name];
}

void Program::updateUniforms()
{
	for (std::pair<std::string, ref_ptr<Uniform>> uniformPair: _uniforms)
	{
		uniformPair.second->updateFor(this);
	}
}

std::string Program::infoLog() const
{
	GLsizei length;
	glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &length);

	std::vector<char> log(length);

	glGetProgramInfoLog(_id, length, &length, log.data());

	return std::string(log.data(), length);
}

void Program::checkLinkStatus()
{
	GLint status = 0;
	glGetProgramiv(_id, GL_LINK_STATUS, &status);

	_linked = (status == GL_TRUE);

	if (!_linked)
	{
		error()
			<< "Linker error:" << std::endl
			<< infoLog();
	}
}

void Program::setShaderStorageBlockBinding(GLuint storageBlockIndex, GLuint storageBlockBinding)
{
	checkDirty();
	glShaderStorageBlockBinding(_id, storageBlockIndex, storageBlockBinding);
}
