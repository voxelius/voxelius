#include "client/pch.hh"

#include "client/world/skybox.hh"

Eigen::Vector3f skybox::sky_color;

void skybox::init(void)
{
    sky_color = Eigen::Vector3f(0.42f, 0.60f, 0.92f);
}
