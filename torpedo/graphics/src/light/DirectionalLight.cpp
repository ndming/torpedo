#include "torpedo/light/DirectionalLight.h"

std::shared_ptr<tpd::DirectionalLight> tpd::DirectionalLight::Builder::build() const {
    return std::make_shared<DirectionalLight>(std::array{ _dirX, _dirY, _dirZ }, std::array{ _r, _g, _b }, _intensity);
}
