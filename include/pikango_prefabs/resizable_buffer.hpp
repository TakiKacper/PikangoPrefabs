#pragma once
#include "pikango/pikango.hpp"

namespace pikango_prefabs_internal
{
    template<size_t(data_size_round_function)(size_t data_size)>
    class resizable_buffer_template;
}

template<size_t(data_size_round_function)(size_t data_size)>
class pikango_prefabs_internal::resizable_buffer_template
{
private:
    pikango::buffer_handle buffer;
    size_t buffer_size;

    pikango::buffer_memory_profile  buffer_memory_profile;
    pikango::buffer_access_profile  buffer_access_profile;

public:
    resizable_buffer_template(
        pikango::command_buffer_handle& command_buffer, 
        size_t                          starter_buffer_size,
        pikango::buffer_memory_profile  memory_profile,
        pikango::buffer_access_profile  access_profile
    )
    {
        buffer_size = data_size_round_function(starter_buffer_size);
        buffer_memory_profile = memory_profile;
        buffer_access_profile = access_profile;

        //create buffer on gpu
        buffer = pikango::new_buffer();
        pikango::begin_command_buffer_recording(command_buffer);
        pikango::cmd::assign_buffer_memory(buffer, buffer_size, buffer_memory_profile, buffer_access_profile);
        pikango::end_command_buffer_recording(command_buffer);
    }

    inline pikango::buffer_handle get_buffer() noexcept
    {
        return buffer;
    }

    inline void cmd_write_buffer(void* data, size_t data_size) noexcept
    {
        if (data_size > buffer_size)
        {
            buffer_size = data_size_round_function(data_size);   
            pikango::cmd::assign_buffer_memory(buffer, buffer_size, buffer_memory_profile, buffer_access_profile);
        }
        pikango::cmd::write_buffer(buffer, data_size, data);
    }

    inline void cmd_write_buffer_region(void* data, size_t data_size, size_t data_offset) noexcept
    {
        if (data_offset + data_size > buffer_size)
        {
            auto bigger_buffer = pikango::new_buffer();
            size_t bigger_buffer_size = data_size_round_function(data_offset + data_size); //since data end is the same as minimal required buffer size

            pikango::cmd::assign_buffer_memory(bigger_buffer, bigger_buffer_size, buffer_memory_profile, buffer_access_profile);
            
            //copy data from previous buffer, till the new data begin
            pikango::cmd::copy_buffer_to_buffer(buffer, bigger_buffer, 0, data_offset, 0);
            
            buffer = bigger_buffer;
            buffer_size = bigger_buffer_size;
        }
        pikango::cmd::write_buffer_region(buffer, data_size, data, data_offset);
    }
};

namespace pikango_prefabs_internal
{
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

namespace pikango_prefabs
{
    using  loose_resizable_buffer = nl_internal::resizable_buffer_template<nl_internal::snap_to_power_of_two>;
    using  tight_resizable_buffer = nl_internal::resizable_buffer_template<nl_internal::return_n>;
}
