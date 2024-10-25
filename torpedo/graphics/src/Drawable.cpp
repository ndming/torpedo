#include "torpedo/graphics/Drawable.h"

std::shared_ptr<tpd::Drawable> tpd::Drawable::Builder::build() {
    return std::make_shared<Drawable>(std::move(_meshes));
}
