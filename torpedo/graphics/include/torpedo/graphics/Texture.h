#pragma once

#include <torpedo/bootstrap/SamplerBuilder.h>
#include <torpedo/foundation/Image.h>

namespace tpd {
    class Texture final {
    public:
        static vk::Sampler getDefaultSampler();

    private:
        static vk::Sampler _defaultSampler;
        friend class Renderer;
    };
}

inline vk::Sampler tpd::Texture::_defaultSampler = {};

// =====================================================================================================================
// INLINE FUNCTION DEFINITIONS
// =====================================================================================================================

inline vk::Sampler tpd::Texture::getDefaultSampler() {
    return _defaultSampler;
}
