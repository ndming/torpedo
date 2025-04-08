#pragma once

namespace tpd {
    template<typename T>
    struct vec4_t {
        using value_type = T;
        value_type value[4];
    };
}