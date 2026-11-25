#include "client/pch.hh"

#include "client/lighting.hh"

#include "client/fog.hh"

const Eigen::Vector3f client_lighting::SKY_COLOR { 1.0f, 1.0f, 1.0f };
const Eigen::Vector3f client_lighting::BLOCK_COLOR { 1.0f, 0.86f, 0.66f };
const float client_lighting::AMBIENT = 0.03f;

Eigen::Vector3f client_lighting::sky_color(void)
{
    return SKY_COLOR * fog::daylight * fog::light_scale;
}
