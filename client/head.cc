#include "client/pch.hh"

#include "client/head.hh"

#include "core/cmdline.hh"
#include "core/cvar/host.hh"
#include "core/cvar/number.hh"
#include "core/exception.hh"
#include "core/identifier.hh"

#include "shared/constant.hh"

#include "client/camera.hh"
#include "client/fog.hh"
#include "client/game.hh"
#include "client/globals.hh"
#include "client/gui/container.hh"
#include "client/gui/slider.hh"
#include "client/settings.hh"
#include "client/shader_program.hh"
#include "client/world/chunk_renderer.hh"
#include "client/world/chunk_vbo.hh"
#include "client/world/cloud_renderer.hh"
#include "client/world/outline.hh"

struct LayerTarget final {
    GLuint fbo { 0 };
    GLuint color { 0 };
    GLuint depth { 0 };
};

static LayerTarget s_diffuse;
static LayerTarget s_clouds;
static LayerTarget s_alpha;
static LayerTarget s_fluid;

static GLuint s_compose_fbo;
static GLuint s_compose_color;

static ShaderProgram s_transparency;
static std::size_t su_DiffuseColor;
static std::size_t su_DiffuseDepth;
static std::size_t su_AlphaColor;
static std::size_t su_AlphaDepth;
static std::size_t su_FluidColor;
static std::size_t su_FluidDepth;
static std::size_t su_CloudColor;
static std::size_t su_CloudDepth;
static std::size_t su_CloudRange;
static std::size_t su_CameraRange;

static cvar::Number<int> s_head_pixel_size { 1, 1, 4 };
static gui::SliderInt s_pixel_size_gui;
static bool s_pixel_size_dirty { true };

static int s_scaled_width;
static int s_scaled_height;

static void destroy_layer(LayerTarget& layer)
{
    if(layer.fbo) {
        glDeleteFramebuffers(1, &layer.fbo);
        glDeleteTextures(1, &layer.color);
        glDeleteTextures(1, &layer.depth);

        layer.fbo = 0;
        layer.color = 0;
        layer.depth = 0;
    }
}

static void create_or_resize_layer(LayerTarget& layer, int width, int height)
{
    if(layer.fbo == 0) {
        glGenFramebuffers(1, &layer.fbo);
        glGenTextures(1, &layer.color);
        glGenTextures(1, &layer.depth);
    }

    glBindTexture(GL_TEXTURE_2D, layer.color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, layer.depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, layer.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, layer.color, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, layer.depth, 0);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    vx::throw_if_not(status == GL_FRAMEBUFFER_COMPLETE, "failed to update layer FBO");
}

static void update_framebuffer(int new_width, int new_height)
{
    auto pixel_size = std::clamp<int>(s_head_pixel_size.value(), 1, 4);
    s_scaled_width = new_width / pixel_size;
    s_scaled_height = new_height / pixel_size;

    create_or_resize_layer(s_diffuse, s_scaled_width, s_scaled_height);
    create_or_resize_layer(s_clouds, s_scaled_width, s_scaled_height);
    create_or_resize_layer(s_alpha, s_scaled_width, s_scaled_height);
    create_or_resize_layer(s_fluid, s_scaled_width, s_scaled_height);

    if(s_compose_fbo == 0) {
        glGenFramebuffers(1, &s_compose_fbo);
        glGenTextures(1, &s_compose_color);
    }

    glBindTexture(GL_TEXTURE_2D, s_compose_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, s_scaled_width, s_scaled_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, s_compose_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_compose_color, 0);

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    vx::throw_if_not(status == GL_FRAMEBUFFER_COMPLETE, "failed to update compose FBO");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void blit_depth(const LayerTarget& src, const LayerTarget& dst)
{
    glBindFramebuffer(GL_READ_FRAMEBUFFER, src.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst.fbo);
    glBlitFramebuffer(0, 0, s_scaled_width, s_scaled_height, 0, 0, s_scaled_width, s_scaled_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

static void begin_diffuse(void)
{
    glBindFramebuffer(GL_FRAMEBUFFER, s_diffuse.fbo);
    glViewport(0, 0, s_scaled_width, s_scaled_height);

    glClearDepth(1.0f);
    glClearColor(fog::color.x(), fog::color.y(), fog::color.z(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void begin_translucent(LayerTarget& layer)
{
    blit_depth(s_diffuse, layer);

    glBindFramebuffer(GL_FRAMEBUFFER, layer.fbo);
    glViewport(0, 0, s_scaled_width, s_scaled_height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

static void begin_cloud_layer(void)
{
    // Clouds go into a layer of their own: the composite lays them over the
    // translucent passes and compares their distance with the ground, so the
    // deck still hides behind terrain even though it has a depth range of its own
    glBindFramebuffer(GL_FRAMEBUFFER, s_clouds.fbo);
    glViewport(0, 0, s_scaled_width, s_scaled_height);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void composite_layers(void)
{
    vx::throw_if_not(s_transparency.update());

    glBindFramebuffer(GL_FRAMEBUFFER, s_compose_fbo);
    glViewport(0, 0, s_scaled_width, s_scaled_height);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);

    glUseProgram(s_transparency.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_diffuse.color);
    glUniform1i(s_transparency.uniforms[su_DiffuseColor].location, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, s_diffuse.depth);
    glUniform1i(s_transparency.uniforms[su_DiffuseDepth].location, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, s_alpha.color);
    glUniform1i(s_transparency.uniforms[su_AlphaColor].location, 2);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, s_alpha.depth);
    glUniform1i(s_transparency.uniforms[su_AlphaDepth].location, 3);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, s_fluid.color);
    glUniform1i(s_transparency.uniforms[su_FluidColor].location, 4);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, s_fluid.depth);
    glUniform1i(s_transparency.uniforms[su_FluidDepth].location, 5);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, s_clouds.color);
    glUniform1i(s_transparency.uniforms[su_CloudColor].location, 6);

    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, s_clouds.depth);
    glUniform1i(s_transparency.uniforms[su_CloudDepth].location, 7);

    glUniform2f(s_transparency.uniforms[su_CloudRange].location, cloud_renderer::z_near, cloud_renderer::z_far);
    glUniform2f(s_transparency.uniforms[su_CameraRange].location, camera::z_near, camera::z_far);

    glDrawArrays(GL_TRIANGLES, 0, 3);

    glDepthMask(GL_TRUE);
}

static void on_sdl_window_event(const SDL_WindowEvent& event)
{
    if(event.type == SDL_EVENT_WINDOW_RESIZED) {
        update_framebuffer(event.data1, event.data2);
        return;
    }
}

static void on_pixel_size_changed(void)
{
    s_pixel_size_dirty = true;
}

void head::init(void)
{
    auto& pixel_size_binding = cvar::bind_var(s_head_pixel_size, "r_pixel_size");
    pixel_size_binding.set_help("Internal render scale, in pixels");
    pixel_size_binding.set_flags(FCVAR_ARCHIVE);
    pixel_size_binding.changed().connect<&on_pixel_size_changed>();

    s_pixel_size_gui.set_range(1, 4).enable_tooltip();
    s_pixel_size_gui.bind("r_pixel_size");
    settings::video.add_child(s_pixel_size_gui, 2);

    globals::gl_context = SDL_GL_CreateContext(globals::window);
    vx::throw_if_not_fmt(globals::gl_context, "SDL_GL_CreateContext failed: {}", SDL_GetError());

    SDL_GL_MakeCurrent(globals::window, globals::gl_context);

    auto glad_ok = gladLoadGL(reinterpret_cast<GLADloadfunc>(&SDL_GL_GetProcAddress));
    vx::throw_if_not(glad_ok, "failed to load OpenGL function pointers");

    msg::log_info("GL_VERSION: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    msg::log_info("GL_RENDERER: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    auto imgui_platform_ok = ImGui_ImplSDL3_InitForOpenGL(globals::window, globals::gl_context);
    vx::throw_if_not(imgui_platform_ok, "ImGui_ImplSDL3_InitForOpenGL failed");

    auto imgui_renderer_ok = ImGui_ImplOpenGL3_Init("#version 330 core");
    vx::throw_if_not(imgui_renderer_ok, "ImGui_ImplOpenGL3_Init failed");

    ImGui::GetIO().IniFilename = nullptr;

    auto transparency_id = Identifier::from_parts(constant::BUILTIN_NAME_SPACE, "transparency");
    auto transparency_ok = s_transparency.setup(transparency_id);
    vx::throw_if_not_fmt(transparency_ok, "{}: setup failed", transparency_id.full_string());

    su_DiffuseColor = s_transparency.add_uniform("u_DiffuseColor");
    su_DiffuseDepth = s_transparency.add_uniform("u_DiffuseDepth");
    su_AlphaColor = s_transparency.add_uniform("u_AlphaColor");
    su_AlphaDepth = s_transparency.add_uniform("u_AlphaDepth");
    su_FluidColor = s_transparency.add_uniform("u_FluidColor");
    su_FluidDepth = s_transparency.add_uniform("u_FluidDepth");
    su_CloudColor = s_transparency.add_uniform("u_CloudColor");
    su_CloudDepth = s_transparency.add_uniform("u_CloudDepth");
    su_CloudRange = s_transparency.add_uniform("u_CloudRange");
    su_CameraRange = s_transparency.add_uniform("u_CameraRange");

    chunk_vbo::init();
    chunk_renderer::init();
    cloud_renderer::init();
    outline::init();

    SDL_ShowWindow(globals::window);

    globals::dispatcher.sink<SDL_WindowEvent>().connect<&on_sdl_window_event>();
}

void head::init_late(void)
{
    // empty
}

void head::shutdown(void)
{
    outline::shutdown();
    cloud_renderer::shutdown();
    chunk_renderer::shutdown();
    chunk_vbo::shutdown();

    s_transparency.destroy();

    destroy_layer(s_diffuse);
    destroy_layer(s_clouds);
    destroy_layer(s_alpha);
    destroy_layer(s_fluid);

    if(s_compose_fbo) {
        glDeleteFramebuffers(1, &s_compose_fbo);
        glDeleteTextures(1, &s_compose_color);

        s_compose_color = 0;
        s_compose_fbo = 0;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(globals::gl_context);
}

bool head::prepare(void)
{
    ZoneScoped;

    int width, height;
    SDL_GetWindowSizeInPixels(globals::window, &width, &height);
    glViewport(0, 0, width, height);

    if(s_pixel_size_dirty || s_diffuse.fbo == 0) {
        update_framebuffer(width, height);
        s_pixel_size_dirty = false;
    }

    glClearColor(0.0f, 0.0f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    return true;
}

void head::render(void)
{
    ZoneScoped;

    begin_diffuse();

    chunk_renderer::prepare();
    chunk_renderer::render_opaque();

    client_game::render();

    begin_cloud_layer();
    cloud_renderer::render();

    begin_translucent(s_alpha);

    chunk_renderer::render_alpha();

    begin_translucent(s_fluid);
    chunk_renderer::render_fluid();

    composite_layers();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, globals::width, globals::height);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_compose_fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, s_scaled_width, s_scaled_height, 0, 0, globals::width, globals::height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void head::present(void)
{
    ZoneScoped;

    ImGui::Render();

    auto draw_data = ImGui::GetDrawData();
    globals::num_draw_calls += draw_data->CmdLists.Size;
    globals::num_draw_vertices += draw_data->TotalVtxCount;

    ImGui_ImplOpenGL3_RenderDrawData(draw_data);

    SDL_GL_SwapWindow(globals::window);
}
