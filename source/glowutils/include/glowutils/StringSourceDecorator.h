#pragma once

#include <glowbase/ChangeListener.h>
#include <glowbase/ref_ptr.h>

#include <glow/AbstractStringSource.h>

#include <glowutils/glowutils_api.h>

namespace glowutils 
{

class GLOWUTILS_API StringSourceDecorator : public glow::AbstractStringSource, protected glowbase::ChangeListener
{
public:
    StringSourceDecorator(glow::AbstractStringSource * source);

    virtual void update();
protected:
    virtual ~StringSourceDecorator();

    virtual void notifyChanged(const Changeable * changeable) override;
protected:
    glowbase::ref_ptr<glow::AbstractStringSource> m_internal;
};

} // namespace glowutils
