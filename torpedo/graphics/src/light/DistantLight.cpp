#include "torpedo/light/DistantLight.h"

std::shared_ptr<tpd::DistantLight> tpd::DistantLight::Builder::build() const {
    return std::make_shared<DistantLight>(std::array{ _dirX, _dirY, _dirZ }, std::array{ _r, _g, _b }, _intensity);
}
