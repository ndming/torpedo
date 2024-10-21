#include "torpedo/light/AmbientLight.h"

std::shared_ptr<tpd::AmbientLight> tpd::AmbientLight::Builder::build() const {
    return std::make_shared<AmbientLight>(std::array{ _r, _g, _b }, _intensity);
}
