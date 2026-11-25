#ifndef BE77D255_1BBD_4613_A663_6597A54D3FC7
#define BE77D255_1BBD_4613_A663_6597A54D3FC7

namespace sky_renderer
{
void init(void);
void shutdown(void);
void render(void);
} // namespace sky_renderer

namespace sky_renderer
{
// The resource system is purged before client_head::shutdown runs, so the
// texture handles have to go first; the GL objects go in shutdown
void release_textures(void);
} // namespace sky_renderer

#endif /* BE77D255_1BBD_4613_A663_6597A54D3FC7 */
