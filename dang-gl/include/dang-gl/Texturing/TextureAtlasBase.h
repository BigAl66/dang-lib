#pragma once

#include "dang-gl/Math/MathTypes.h"
#include "dang-gl/Texturing/TextureAtlasTiles.h"
#include "dang-gl/global.h"

namespace dang::gl {

/*

The TextureBase concept:

- Move-constructible
- using ImageData = ...;
- bool resize(GLsizei required_size, GLsizei layers, GLsizei mipmap_levels)
    -> protected, resizes the texture
- void modify(const ImageData& image, ivec3 offset, GLint mipmap_level)
    -> protected, modifies the texture at a given spot

*/

template <typename TTextureBase>
class BasicFrozenTextureAtlas;

template <typename TTextureBase>
class TextureAtlasBase : public TTextureBase {
public:
    using ImageData = typename TTextureBase::ImageData;
    using Tiles = TextureAtlasTiles<ImageData>;
    using TileHandle = typename Tiles::TileHandle;
    using Frozen = BasicFrozenTextureAtlas<TTextureBase>;

    TextureAtlasBase(const TextureAtlasLimits& limits)
        : tiles_(limits)
    {}

    TextureAtlasTileBorderGeneration guessTileBorderGeneration(GLsizei size) const
    {
        return tiles_.guessTileBorderGeneration(size);
    }

    TextureAtlasTileBorderGeneration guessTileBorderGeneration(svec2 size) const
    {
        return tiles_.guessTileBorderGeneration(size);
    }

    TextureAtlasTileBorderGeneration defaultBorderGeneration() const { return tiles_.defaultBorderGeneration(); }

    void setDefaultBorderGeneration(TextureAtlasTileBorderGeneration border)
    {
        tiles_.setDefaultBorderGeneration(border);
    }

    [[nodiscard]] TileHandle add(ImageData image_data,
                                 std::optional<TextureAtlasTileBorderGeneration> border = std::nullopt)
    {
        return tiles_.add(std::move(image_data), border);
    }

    void add(std::string name,
             ImageData image_data,
             std::optional<TextureAtlasTileBorderGeneration> border = std::nullopt)
    {
        tiles_.add(std::move(name), std::move(image_data), border);
    }

    [[nodiscard]] TileHandle addWithHandle(std::string name,
                                           ImageData image_data,
                                           std::optional<TextureAtlasTileBorderGeneration> border = std::nullopt)
    {
        return tiles_.addWithHandle(std::move(name), std::move(image_data), border);
    }

    [[nodiscard]] bool exists(const TileHandle& tile_handle) const { return tiles_.exists(tile_handle); }
    [[nodiscard]] bool exists(const std::string& name) const { return tiles_.exists(name); }
    [[nodiscard]] TileHandle operator[](const std::string& name) const { return tiles_[name]; }

    bool tryRemove(const TileHandle& tile_handle) { return tiles_.tryRemove(tile_handle); }
    bool tryRemove(const std::string& name) { return tiles_.tryRemove(name); }
    void remove(const TileHandle& tile_handle) { return tiles_.remove(tile_handle); }
    void remove(const std::string& name) { return tiles_.remove(name); }

    void updateTexture() { return updateTextureHelper<false>(); }
    Frozen freeze() && { return updateTextureHelper<true>(); }

private:
    template <bool v_freeze>
    std::conditional_t<v_freeze, Frozen, void> updateTextureHelper()
    {
        // TODO: C++20 replace with std::bind_front
        using namespace std::placeholders;

        auto resize = std::bind(&TextureAtlasBase::resize, this, _1, _2, _3);
        auto modify = std::bind(&TextureAtlasBase::modify, this, _1, _2, _3);

        if constexpr (v_freeze)
            return Frozen(std::move(tiles_).freeze(resize, modify), std::move(*this));
        else
            tiles_.updateTexture(resize, modify);
    }

    Tiles tiles_;
};

template <typename TTextureBase>
class BasicFrozenTextureAtlas : public TTextureBase {
public:
    using ImageData = typename TTextureBase::ImageData;
    using Tiles = FrozenTextureAtlasTiles<ImageData>;
    using TileHandle = typename Tiles::TileHandle;

    friend class TextureAtlasBase<TTextureBase>;

    [[nodiscard]] bool exists(const TileHandle& tile_handle) const { return tiles_.exists(tile_handle); }
    [[nodiscard]] bool exists(const std::string& name) const { return tiles_.exists(name); }
    [[nodiscard]] TileHandle operator[](const std::string& name) const { return tiles_[name]; }

private:
    BasicFrozenTextureAtlas(Tiles&& tiles, TTextureBase&& texture)
        : TTextureBase(std::move(texture))
        , tiles_(std::move(tiles))
    {}

    Tiles tiles_;
};

} // namespace dang::gl
