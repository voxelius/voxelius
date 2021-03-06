/*
 * sprite_renderer.hh
 * Created: 2021-03-08, 18:48:54.
 * Copyright (C) 2021, Kirill GPRB.
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once
#include <thorn/gl/pipeline.hh>
#include <thorn/gl/texture.hh>
#include <thorn/gl/vertex_array.hh>
#include <thorn/math/view.hh>
#include <vector>

namespace thorn::render
{
/**
 * @brief Sprite renderer.
 * 
 */
class SpriteRenderer {
public:
    /**
     * @brief Uniform Buffer structure.
     * 
     */
    struct alignas(16) ubo_s {
        float4x4_t projection;
        float4x4_t scale;
        float4x4_t view;
    };

    const std::string DEFAULT_VERT = "assets/shaders/sprite.vert.spv";
    const std::string DEFAULT_FRAG = "assets/shaders/sprite.frag.spv";

public:
    /**
     * @brief Constructor.
     * 
     */
    SpriteRenderer();

    /**
     * @brief Loads a default shader binary from file.
     * 
     * @tparam T Shader stage.
     * @return true if succeeded, false otherwise.
     */
    template<gl::ShaderStage T>
    bool loadDefaultShader();

    /**
     * @brief Loads a custom shader binary.
     * 
     * @tparam T Shader stage.
     * @param binary SPIR-V binary.
     * @return true if succeeded, false otherwise.
     */
    template<gl::ShaderStage T>
    bool loadShader(const std::vector<uint8_t> &binary);

    /**
     * @brief Sets the view.
     * 
     * @param view
     */
    void setView(math::View &view);

    /**
     * @brief Draws sprites.
     * 
     * @param instances Intance vector.
     * @param texture Sprite texture.
     * @param size Sprite/Texture size in pixels.
     */
    void draw(const std::vector<float4x4_t> &instances, const gl::Texture2D &texture, const float2_t &size);

private:
    gl::VertexShader vert;
    gl::FragmentShader frag;
    gl::Pipeline pipeline;

    gl::Buffer vbo, ebo;
    gl::VertexArray vao;

    gl::Buffer ubo;
    gl::Buffer ssbo;

    std::vector<float4x4_t> instances;
};
} // namespace thorn::render
