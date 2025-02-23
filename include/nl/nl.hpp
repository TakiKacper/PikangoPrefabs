#pragma once
#include "pikango/pikango.hpp"
#include "glm/glm.hpp"

namespace nl_internal
{
    template<size_t(data_size_round_function)(size_t data_size)>
    class resizable_buffer_template;

    inline size_t snap_to_power_of_two(size_t n) noexcept
    {
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n |= n >> 32;
        return n + 1;
    }

    inline size_t return_n(size_t n) noexcept
    {
        return n;
    }
}

namespace nl
{
    const glm::vec3 world_up = {0, 1, 0};

    struct camera;

    using  loose_resizable_buffer = nl_internal::resizable_buffer_template<nl_internal::snap_to_power_of_two>;
    using  tight_resizable_buffer = nl_internal::resizable_buffer_template<nl_internal::return_n>;

    class  partitioned_buffer;
    class  paged_buffer;

    struct material;
    struct static_model;
}

#include "camera.hpp"

#include "resizable_buffer_template.hpp"

#include "partitioned_buffer.hpp"
#include "paged_buffer.hpp"

#include "static_model.hpp"

