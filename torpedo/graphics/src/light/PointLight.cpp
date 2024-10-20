#include "torpedo/light/PointLight.h"

std::shared_ptr<tpd::PointLight> tpd::PointLight::Builder::build() const {
    return std::make_shared<PointLight>(std::array{ _posX, _posY, _posZ }, _decay, std::array{ _r, _g, _b }, _intensity);
}
