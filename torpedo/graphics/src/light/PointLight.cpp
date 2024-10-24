#include "torpedo/light/PointLight.h"

#include <glm/gtc/matrix_transform.hpp>

std::shared_ptr<tpd::PointLight> tpd::PointLight::Builder::build() const {
    return std::make_shared<PointLight>(_position, _decay, std::array{ _r, _g, _b }, _intensity);
}

void tpd::PointLight::setPosition(const float x, const float y, const float z) {
    _position = { x, y, z };
    setTransform(translate(glm::mat4{ 1.0f }, _position));
}
