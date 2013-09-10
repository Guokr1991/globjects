
#include <iostream>

#include <glow/Error.h>
#include <glow/logging.h>
#include <glow/Buffer.h>
#include <glow/ObjectVisitor.h>

namespace glow
{

Buffer::Buffer()
: Object(genBuffer())
, _target(0)
{
}

Buffer::Buffer(GLenum target)
: Object(genBuffer())
, _target(target)
{
}

Buffer::Buffer(GLuint id, GLenum target)
: Object(id, false)
, _target(target)
{
}

GLuint Buffer::genBuffer()
{
	GLuint id = 0;

	glGenBuffers(1, &id);
	CheckGLError();

	return id;
}

Buffer::~Buffer()
{
	if (ownsGLObject())
	{
		glDeleteBuffers(1, &m_id);
		CheckGLError();
	}
}

void Buffer::accept(ObjectVisitor& visitor)
{
	visitor.visitBuffer(this);
}

void Buffer::bind()
{
	glBindBuffer(_target, m_id);
	CheckGLError();
}

void Buffer::bind(GLenum target)
{
	_target = target;
	bind();
}

void Buffer::unbind()
{
	glBindBuffer(_target, 0);
	CheckGLError();
}

void* Buffer::map(GLenum access)
{
	bind();

	void* result = glMapBuffer(_target, access);
	CheckGLError();
	return result;
}

void* Buffer::map(GLenum target, GLenum access)
{
	bind(target);

	void* result = glMapBuffer(target, access);
	CheckGLError();
	return result;
}

void Buffer::unmap()
{
	glUnmapBuffer(_target);
	CheckGLError();
}

void Buffer::setData(const AbstractArray& data, GLenum usage)
{
	setData(data.rawSize(), data.rawData(), usage);
}

void Buffer::setData(GLsizei size, const GLvoid* data, GLenum usage)
{
	bind();
	glBufferData(_target, size, data, usage);
	CheckGLError();
}

GLint Buffer::getParameter(GLenum pname)
{
	bind();

	GLint value = 0;

	glGetBufferParameteriv(_target, pname, &value);
	CheckGLError();

	return value;
}

void Buffer::drawArrays(GLenum mode, GLint first, GLsizei count)
{
	bind();
	glDrawArrays(mode, first, count);
	CheckGLError();
}

void Buffer::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid* indices)
{
	bind();
	glDrawElements(mode, count, type, indices);
	CheckGLError();
}

void Buffer::bindBase(GLenum target, GLuint index)
{
	glBindBufferBase(target, index, m_id);
	CheckGLError();
}

void Buffer::bindRange(GLenum target, GLuint index, GLintptr offset, GLsizeiptr size)
{
	glBindBufferRange(target, index, m_id, offset, size);
	CheckGLError();
}

void Buffer::copySubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
	bind(readTarget);
	glCopyBufferSubData(readTarget, writeTarget, readOffset, writeOffset, size);
	CheckGLError();
}

void Buffer::copySubData(GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
	copySubData(GL_COPY_READ_BUFFER, writeTarget, readOffset, writeOffset, size);
}

void Buffer::copySubData(GLenum writeTarget, GLsizeiptr size)
{
	copySubData(GL_COPY_READ_BUFFER, writeTarget, 0, 0, size);
}

void Buffer::copySubData(glow::Buffer* buffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
	buffer->bind(GL_COPY_WRITE_BUFFER);
	copySubData(GL_COPY_WRITE_BUFFER, readOffset, writeOffset, size);
}

void Buffer::copySubData(glow::Buffer* buffer, GLsizeiptr size)
{
	copySubData(buffer, 0, 0, size);
}

void Buffer::copyData(glow::Buffer* buffer, GLsizeiptr size, GLenum usage)
{
	buffer->bind(GL_COPY_WRITE_BUFFER);
	buffer->setData(static_cast<GLsizei>(size), nullptr, usage);
	copySubData(buffer, 0, 0, size);
}

void Buffer::clearData(GLenum internalformat, GLenum format, GLenum type, const void* data)
{
    bind();

    glClearBufferData(_target, internalformat, format, type, data);
    CheckGLError();
}

void Buffer::clearData(GLenum target, GLenum internalformat, GLenum format, GLenum type, const void* data)
{
    _target = target;

    clearData(internalformat, format, type, data);
}

} // namespace glow
