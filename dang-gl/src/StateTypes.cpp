#include "pch.h"
#include "StateTypes.h"

namespace dang::gl
{

bool operator==(const BlendFactor& lhs, const BlendFactor& rhs)
{
    return std::tie(lhs.src, lhs.dst) == std::tie(rhs.src, rhs.dst);
}

bool operator!=(const BlendFactor& lhs, const BlendFactor& rhs)
{
    return !(lhs == rhs);
}

std::tuple<GLenum, GLenum> BlendFactor::toTuple() const
{
    return { static_cast<GLenum>(src), static_cast<GLenum>(dst) };
}

bool operator==(const SampleCoverage& lhs, const SampleCoverage& rhs)
{
    return std::tie(lhs.value, lhs.invert) == std::tie(rhs.value, rhs.invert);
}

bool operator!=(const SampleCoverage& lhs, const SampleCoverage& rhs)
{
    return !(lhs == rhs);
}

std::tuple<GLclampf, GLboolean> SampleCoverage::toTuple() const
{
    return { value, invert };
}

bool operator==(const Scissor& lhs, const Scissor& rhs)
{
    return lhs.bounds == rhs.bounds;
}

bool operator!=(const Scissor& lhs, const Scissor& rhs)
{
    return !(lhs == rhs);
}

std::tuple<GLint, GLint, GLsizei, GLsizei> Scissor::toTuple() const
{
    const auto& size = bounds.size();
    return { bounds.low.x(), bounds.low.y(), size.x(), size.y() };
}
                             
bool operator==(const StencilFunc& lhs, const StencilFunc& rhs)
{
    return std::tie(lhs.func, lhs.ref, lhs.mask) == std::tie(rhs.func, rhs.ref, rhs.mask);
}

bool operator!=(const StencilFunc& lhs, const StencilFunc& rhs)
{
    return !(lhs == rhs);
}

std::tuple<GLenum, GLint, GLuint> StencilFunc::toTuple() const
{
    return { static_cast<GLenum>(func), ref, mask };
}

bool operator==(const StencilOp& lhs, const StencilOp& rhs)
{
    return std::tie(lhs.sfail, lhs.dpfail, lhs.dppass) == std::tie(rhs.sfail, rhs.dpfail, rhs.dppass);
}

bool operator!=(const StencilOp& lhs, const StencilOp& rhs)
{
    return !(lhs == rhs);
}

std::tuple<GLenum, GLenum, GLenum> StencilOp::toTuple() const
{
    return { static_cast<GLenum>(sfail), static_cast<GLenum>(dpfail), static_cast<GLenum>(dppass) };
}

}
