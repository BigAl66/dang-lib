#include "dang-gl/Texturing/TextureAtlas.h"

namespace dang::gl {

TextureAtlas::TextureAtlas(std::optional<GLsizei> max_texture_size, std::optional<GLsizei> max_layer_count)
    : tiles_(max_texture_size.value_or(context()->max_3d_texture_size),
             max_layer_count.value_or(context()->max_array_texture_layers))
{
    assert(!max_texture_size || max_texture_size >= 1 && max_texture_size <= context()->max_3d_texture_size);
    assert(!max_layer_count || max_layer_count >= 1 && max_layer_count <= context()->max_array_texture_layers);
}

TextureAtlas::TileBorderGeneration TextureAtlas::guessTileBorderGeneration(GLsizei size) const
{
    return tiles_.guessTileBorderGeneration(size);
}

TextureAtlas::TileBorderGeneration TextureAtlas::guessTileBorderGeneration(svec2 size) const
{
    return tiles_.guessTileBorderGeneration(size);
}

TextureAtlas::TileBorderGeneration TextureAtlas::defaultBorderGeneration() const
{
    return tiles_.defaultBorderGeneration();
}

void TextureAtlas::setDefaultBorderGeneration(TileBorderGeneration border)
{
    tiles_.setDefaultBorderGeneration(border);
}

bool TextureAtlas::add(std::string name, Image2D image, std::optional<TileBorderGeneration> border)
{
    return tiles_.add(std::move(name), std::move(image), border);
}

TextureAtlas::TileHandle TextureAtlas::addWithHandle(std::string name,
                                                     Image2D image,
                                                     std::optional<TileBorderGeneration> border)
{
    return tiles_.addWithHandle(std::move(name), std::move(image), border);
}

bool TextureAtlas::exists(const std::string& name) const { return tiles_.exists(name); }

TextureAtlas::TileHandle TextureAtlas::operator[](const std::string& name) const { return tiles_[name]; }

bool TextureAtlas::remove(const std::string& name) { return tiles_.remove(name); }

void TextureAtlas::updateTexture()
{
    auto resize = [&](GLsizei required_size, GLsizei layers, GLsizei mipmap_levels) {
        assert(texture_.size().x() == texture_.size().y());
        if (required_size == texture_.size().x() && layers == texture_.size().z())
            return false;
        texture_ = Texture2DArray({required_size, required_size, layers}, mipmap_levels);
        return true;
    };

    auto modify = [&](const Image2D& image, ivec3 offset, GLint mipmap_level) {
        return texture_.modify(image, offset, mipmap_level);
    };

    tiles_.updateTexture(resize, modify);
}

} // namespace dang::gl
