#include <globjects/Program.h>

#include <cassert>
#include <vector>

#include <glbinding/gl/functions.h>
#include <glbinding/gl/extension.h>
#include <glbinding/gl/boolean.h>

#include <globjects/logging.h>
#include <globjects/globjects.h>
#include <globjects/Uniform.h>
#include <globjects/ObjectVisitor.h>
#include <globjects/Shader.h>
#include <globjects/ProgramBinary.h>
#include <globjects/Buffer.h>

#include "Resource.h"
#include "registry/ImplementationRegistry.h"
#include "implementations/AbstractProgramBinaryImplementation.h"

namespace {

const glo::AbstractProgramBinaryImplementation & binaryImplementation()
{
    return glo::ImplementationRegistry::current().programBinaryImplementation();
}

}

namespace glo
{

Program::Program()
: Object(new ProgramResource)
, m_linked(false)
, m_dirty(true)
{
}

Program::Program(ProgramBinary * binary)
: Program()
{
    setBinary(binary);
}

Program::~Program()
{
    for (ref_ptr<Shader> shader: std::set<ref_ptr<Shader>>(m_shaders))
	{
		detach(shader);
	}

    for (std::pair<LocationIdentity, ref_ptr<AbstractUniform>> uniformPair : m_uniforms)
    {
        uniformPair.second->deregisterProgram(this);
    }
}

void Program::accept(ObjectVisitor& visitor)
{
	visitor.visitProgram(this);
}

void Program::use() const
{
	checkDirty();

    if (!isLinked())
        return;

    gl::glUseProgram(id());
}

void Program::release() const
{
    if (!isLinked())
        return;

    gl::glUseProgram(0);
}

bool Program::isUsed() const
{
    gl::GLuint currentProgram = static_cast<gl::GLuint>(getInteger(gl::GL_CURRENT_PROGRAM));

    return currentProgram > 0 && currentProgram == id();
}

bool Program::isLinked() const
{
	return m_linked;
}

void Program::invalidate() const
{
	m_dirty = true;
}

void Program::notifyChanged(const Changeable *)
{
	invalidate();
}

void Program::checkDirty() const
{
	if (m_dirty)
	{
		link();
	}
}

void Program::attach()
{
}

void Program::detach(Shader * shader)
{
    assert(shader != nullptr);

    gl::glDetachShader(id(), shader->id());

	shader->deregisterListener(this);
	m_shaders.erase(shader);

	invalidate();
}

std::set<Shader*> Program::shaders() const
{
	std::set<Shader*> shaders;
    for (ref_ptr<Shader> shader: m_shaders)
		shaders.insert(shader);
	return shaders;
}

void Program::link() const
{
    m_linked = false;

    if (!binaryImplementation().updateProgramLinkSource(this))
        return;

    gl::glLinkProgram(id());

    m_linked = checkLinkStatus();
	m_dirty = false;

    updateUniforms();
    updateUniformBlockBindings();
}

bool Program::compileAttachedShaders() const
{
    for (Shader* shader : shaders())
    {
        if (!shader->isCompiled())
        {
            // Some drivers (e.g. nvidia-331 on Ubuntu 13.04 automatically compile shaders during program linkage)
            // but we don't want to depend on such behavior
            shader->compile();

            if (!shader->isCompiled())
            {
                return false;
            }
        }
    }

    return true;
}

bool Program::checkLinkStatus() const
{
    if (gl::GL_FALSE == static_cast<gl::GLboolean>(get(gl::GL_LINK_STATUS)))
    {
        critical()
            << "Linker error:" << std::endl
            << infoLog();
        return false;
    }
    return true;
}

void Program::bindFragDataLocation(gl::GLuint index, const std::string & name) const
{
    gl::glBindFragDataLocation(id(), index, name.c_str());
}

void Program::bindAttributeLocation(gl::GLuint index, const std::string & name) const
{
    gl::glBindAttribLocation(id(), index, name.c_str());
}

gl::GLint Program::getFragDataLocation(const std::string & name) const
{
    return gl::glGetFragDataLocation(id(), name.c_str());
}

gl::GLint Program::getFragDataIndex(const std::string & name) const
{
    return gl::glGetFragDataIndex(id(), name.c_str());
}

gl::GLint Program::getUniformLocation(const std::string& name) const
{
	checkDirty();
    if (!m_linked)
        return -1;

    return gl::glGetUniformLocation(id(), name.c_str());
}

std::vector<gl::GLint> Program::getAttributeLocations(const std::vector<std::string> & names) const
{
    std::vector<gl::GLint> locations(names.size());
    for (unsigned i = 0; i<names.size(); ++i)
    {
        locations[i] = getAttributeLocation(names[i]);
    }
    return locations;
}

std::vector<gl::GLint> Program::getUniformLocations(const std::vector<std::string> & names) const
{
    std::vector<gl::GLint> locations(names.size());
    for (unsigned i = 0; i<names.size(); ++i)
    {
        locations[i] = getUniformLocation(names[i]);
    }
    return locations;
}

gl::GLint Program::getAttributeLocation(const std::string & name) const
{
	checkDirty();
    if (!m_linked)
        return -1;

    return gl::glGetAttribLocation(id(), name.c_str());
}

gl::GLuint Program::getResourceIndex(gl::GLenum programInterface, const std::string & name) const
{
	checkDirty();

    return gl::glGetProgramResourceIndex(id(), programInterface, name.c_str());
}

gl::GLuint Program::getUniformBlockIndex(const std::string& name) const
{
    checkDirty();

    return gl::glGetUniformBlockIndex(id(), name.c_str());
}

void Program::getActiveUniforms(gl::GLsizei uniformCount, const gl::GLuint * uniformIndices, gl::GLenum pname, gl::GLint * params) const
{
    checkDirty();

    gl::glGetActiveUniformsiv(id(), uniformCount, uniformIndices, pname, params);
}

std::vector<gl::GLint> Program::getActiveUniforms(const std::vector<gl::GLuint> & uniformIndices, gl::GLenum pname) const
{
    std::vector<gl::GLint> result(uniformIndices.size());
    getActiveUniforms(static_cast<gl::GLint>(uniformIndices.size()), uniformIndices.data(), pname, result.data());
    return result;
}

std::vector<gl::GLint> Program::getActiveUniforms(const std::vector<gl::GLint> & uniformIndices, gl::GLenum pname) const
{
    std::vector<gl::GLuint> indices(uniformIndices.size());
    for (unsigned i=0; i<uniformIndices.size(); ++i)
        indices[i] = static_cast<gl::GLuint>(uniformIndices[i]);
    return getActiveUniforms(indices, pname);
}

gl::GLint Program::getActiveUniform(gl::GLuint uniformIndex, gl::GLenum pname) const
{
    gl::GLint result = 0;
    getActiveUniforms(1, &uniformIndex, pname, &result);
    return result;
}

std::string Program::getActiveUniformName(gl::GLuint uniformIndex) const
{
    checkDirty();

    gl::GLint length = getActiveUniform(uniformIndex, gl::GL_UNIFORM_NAME_LENGTH);
    std::vector<char> name(length);
    gl::glGetActiveUniformName(id(), uniformIndex, length, nullptr, name.data());

    return std::string(name.data(), length);
}

UniformBlock * Program::uniformBlock(gl::GLuint uniformBlockIndex)
{
    return getUniformBlockByIdentity(uniformBlockIndex);
}

UniformBlock * Program::uniformBlock(const std::string& name)
{
    return getUniformBlockByIdentity(name);
}

UniformBlock * Program::getUniformBlockByIdentity(const LocationIdentity & identity)
{
    checkDirty();

    if (m_uniformBlocks.find(identity) == m_uniformBlocks.end())
    {
        m_uniformBlocks[identity] = UniformBlock(this, identity);
    }

    return &m_uniformBlocks[identity];
}

void Program::addUniform(AbstractUniform * uniform)
{
    assert(uniform != nullptr);

    ref_ptr<AbstractUniform>& uniformReference = m_uniforms[uniform->identity()];

	if (uniformReference)
	{
		uniformReference->deregisterProgram(this);
	}

	uniformReference = uniform;

	uniform->registerProgram(this);

	if (m_linked)
	{
		uniform->update(this);
	}
}

void Program::updateUniforms() const
{
	// Note: uniform update will check if program is linked
    for (std::pair<LocationIdentity, ref_ptr<AbstractUniform>> uniformPair : m_uniforms)
	{
		uniformPair.second->update(this);
	}
}

void Program::updateUniformBlockBindings() const
{
    for (std::pair<LocationIdentity, UniformBlock> pair : m_uniformBlocks)
    {
        pair.second.updateBinding();
    }
}

void Program::setBinary(ProgramBinary * binary)
{
    if (m_binary == binary)
        return;

    if (m_binary)
        m_binary->deregisterListener(this);

    m_binary = binary;

    if (m_binary)
        m_binary->registerListener(this);
}

ProgramBinary * Program::getBinary() const
{
    return binaryImplementation().getProgramBinary(this);
}

gl::GLint Program::get(gl::GLenum pname) const
{
    gl::GLint value = 0;
    gl::glGetProgramiv(id(), pname, &value);


	return value;
}

const std::string Program::infoLog() const
{
    gl::GLint length = get(gl::GL_INFO_LOG_LENGTH);

    if (length == 0)
    {
        return std::string();
    }

    std::vector<char> log(length);

    gl::glGetProgramInfoLog(id(), length, &length, log.data());

	return std::string(log.data(), length);
}

void Program::dispatchCompute(const glm::uvec3 & numGroups)
{
    dispatchCompute(numGroups.x, numGroups.y, numGroups.z);
}

void Program::dispatchCompute(gl::GLuint numGroupsX, gl::GLuint numGroupsY, gl::GLuint numGroupsZ)
{
    use();

    if (!m_linked)
        return;

    gl::glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}

void Program::dispatchComputeGroupSize(gl::GLuint numGroupsX, gl::GLuint numGroupsY, gl::GLuint numGroupsZ, gl::GLuint groupSizeX, gl::GLuint groupSizeY, gl::GLuint groupSizeZ)
{
    use();

    if (!m_linked)
        return;

    gl::glDispatchComputeGroupSizeARB(numGroupsX, numGroupsY, numGroupsZ, groupSizeX, groupSizeY, groupSizeZ);
}

void Program::dispatchComputeGroupSize(const glm::uvec3 & numGroups, const glm::uvec3 & groupSizes)
{
    dispatchComputeGroupSize(numGroups.x, numGroups.y, numGroups.z, groupSizes.x, groupSizes.y, groupSizes.z);
}

void Program::setShaderStorageBlockBinding(gl::GLuint storageBlockIndex, gl::GLuint storageBlockBinding) const
{
	checkDirty();
    if (!m_linked)
        return;

    gl::glShaderStorageBlockBinding(id(), storageBlockIndex, storageBlockBinding);
}

} // namespace glo