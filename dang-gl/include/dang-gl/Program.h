#pragma once

#include "dang-utils/enum.h"
#include "dang-utils/NonCopyable.h"

#include <map>

#include "ObjectTypes.h"
#include "Object.h"
#include "DataType.h"
#include "UniformWrapper.h"

namespace dang::gl
{

enum class DataType;

enum class ShaderType {
    Vertex,
    Fragment,
    Geometry,
    TesselationControl,
    TesselationEvaluation,
    Compute,
    COUNT
};

constexpr dutils::EnumArray<ShaderType, GLenum> ShaderTypesGL
{
    GL_VERTEX_SHADER,
    GL_FRAGMENT_SHADER,
    GL_GEOMETRY_SHADER,
    GL_TESS_CONTROL_SHADER,
    GL_TESS_EVALUATION_SHADER,
    GL_COMPUTE_SHADER
};

const dutils::EnumArray<ShaderType, std::string> ShaderTypeNames
{
    "Vertex-Shader",
    "Fragment-Shader",
    "Geometry-Shader",
    "Tesselation-Control-Shader",
    "Tesselation-Evaluation-Shader",
    "Compute-Shader"
};

class ShaderError : public std::runtime_error {
public:
    ShaderError(const std::string& info_log) : std::runtime_error(info_log) {}
};

class ShaderCompilationError : public ShaderError {
public:
    ShaderCompilationError(ShaderType type, const std::string& info_log)
        : ShaderError(ShaderTypeNames[type] + "\n" + info_log)
        , type_(type)
    {
    }

    ShaderType type() const { return type_; }

private:
    ShaderType type_;
};

class ShaderLinkError : public ShaderError {
public:
    ShaderLinkError() = default;
    ShaderLinkError(const std::string& info_log)
        : ShaderError("Shader-Linking\n" + info_log)
    {
    }
};

class UniformError : public std::runtime_error {
public:
    UniformError(const std::string& name)
        : std::runtime_error(name + " missing or type-mismatch")
    {
    }
};

struct ProgramInfo : public ObjectInfo {
    static GLuint create();
    static void destroy(GLuint handle);
    static void bind(GLuint handle);

    static constexpr ObjectType Type = ObjectType::Program;
};

class Program;

class ShaderVariable : public dutils::NonCopyable {
public:
    ShaderVariable(Program& program, GLint count, DataType type, std::string name, GLint location);

    Program& program() const;

    GLint count() const;
    DataType type() const;
    const std::string& name() const;
    GLint location() const;

private:
    Program& program_;
    GLint count_;
    DataType type_;
    std::string name_;
    GLint location_;
};

class ShaderAttribute : public ShaderVariable {
public:
    ShaderAttribute(Program& program, GLint count, DataType type, std::string name);
};

class ShaderUniformBase : public ShaderVariable {
public:
    ShaderUniformBase(Program& program, GLint count, DataType type, std::string name);
    virtual ~ShaderUniformBase() {}

    static std::unique_ptr<ShaderUniformBase> create(Program& program, GLint count, DataType type, std::string name);
};

template <typename T>
class ShaderUniform : public ShaderUniformBase {
public:
    ShaderUniform(Program& program, GLint count, DataType type, std::string name);
    ShaderUniform(Program& program, std::string name);

    void force(const T& value, GLint index = 0);
    void set(const T& value, GLint index = 0);
    T get(GLint index = 0);

    ShaderUniform& operator=(const T& value);
    operator T();

private:
    std::vector<T> values_;
};

class Program : public Object<ProgramInfo> {
public:
    void addShader(ShaderType type, std::string shader_code);

    void link();

    template <typename T>
    ShaderUniform<T>& uniform(std::string name);

private:
    void checkShaderStatusAndInfoLog(GLuint shader_handle, ShaderType type);
    void checkLinkStatusAndInfoLog();

    void loadAttributeLocations();
    void loadUniformLocations();

    std::vector<GLuint> shader_handles_;
    std::map<std::string, ShaderAttribute> attributes_;
    std::map<std::string, std::unique_ptr<ShaderUniformBase>> uniforms_;
};

template<typename T>
inline ShaderUniform<T>::ShaderUniform(Program& program, GLint count, DataType type, std::string name)
    : ShaderUniformBase(program, count, type, name)
    , values_(count)
{
    for (GLint index = 0; index < count; index++)
        values_[index] = UniformWrapper<T>::get(program.handle(), location() + index);
}

template<typename T>
inline ShaderUniform<T>::ShaderUniform(Program& program, std::string name)
    : ShaderUniformBase(program, 1, DataType::None, std::move(name))
{
}

template<typename T>
inline void ShaderUniform<T>::force(const T& value, GLint index)
{
    if (location() != -1) {
        program().bind();
        UniformWrapper<T>::set(location() + index, value);
    }
    values_[index] = value;
}

template<typename T>
inline void ShaderUniform<T>::set(const T& value, GLint index)
{
    if (value == values_[index])
        return;
    force(value);
}

template<typename T>
inline T ShaderUniform<T>::get(GLint index)
{
    return values_[index];
}

template<typename T>
inline ShaderUniform<T>& ShaderUniform<T>::operator=(const T& value)
{
    set(value);
    return *this;
}

template<typename T>
inline ShaderUniform<T>::operator T()
{
    return get();
}

template<typename T>
inline ShaderUniform<T>& Program::uniform(std::string name)
{
    auto pos = uniforms_.find(name);
    if (pos == uniforms_.end()) {
        auto uniform = std::make_unique<ShaderUniform<T>>(*this, name);
        ShaderUniform<T>& result = *uniform;
        uniforms_.emplace(name, std::move(uniform));
        return result;
    }
    ShaderUniformBase& shader_uniform = *pos->second;
    return dynamic_cast<ShaderUniform<T>&>(shader_uniform);
}

}
