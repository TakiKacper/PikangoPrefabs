#pragma once
#include "pikango/pikango.hpp"

#include <vector>

namespace nl
{
    class partitioned_buffer;
}

class nl::partitioned_buffer
{
private:
    pikango::buffer_handle buffer;

    size_t  partitions_amount;
    size_t  total_partitions_size;

    std::vector<size_t> partitions_sizes_array;
    std::vector<size_t> partitions_offsets_array;

public:
    partitioned_buffer(
        pikango::command_buffer_handle& command_buffer, 
        std::initializer_list<size_t>   partitions_sizes,
        pikango::buffer_memory_profile  memory_profile,
        pikango::buffer_access_profile  access_profile
    )
    {
        partitions_amount = partitions_sizes.size();

        //calculate sizes and offsets lookup for each partition
        partitions_sizes_array.resize(partitions_amount);
        partitions_offsets_array.resize(partitions_amount);

        size_t offset = 0;
        auto itr = partitions_sizes.begin();
        for (size_t i = 0; i < partitions_amount; i++)
        {
            partitions_sizes_array[i] = *itr;
            partitions_offsets_array[i] = offset;

            offset += *itr;
            itr++;
        }

        total_partitions_size = offset;

        //create buffer on gpu
        buffer = pikango::new_buffer();
        pikango::begin_command_buffer_recording(command_buffer);
        pikango::cmd::assign_buffer_memory(buffer, total_partitions_size, memory_profile, access_profile);
        pikango::end_command_buffer_recording(command_buffer);
    }

    inline pikango::buffer_handle get_buffer() noexcept
    {
        return buffer;
    }

    inline const size_t& get_paritions_amount() const noexcept
    {
        return partitions_amount;
    }

    inline const size_t& get_partition_size(size_t partition) const noexcept
    {
        if (partition >= partitions_amount) return SIZE_MAX;
        return partitions_sizes_array[partition];
    }

    inline const size_t& get_partition_offset(size_t partition) const noexcept
    {
        if (partition >= partitions_amount) return SIZE_MAX;
        return partitions_offsets_array[partition];
    }

    inline void cmd_write_parition(size_t partition, void* data) noexcept
    {
        pikango::cmd::write_buffer_region(
            buffer, 
            partitions_sizes_array[partition], 
            data, 
            partitions_offsets_array[partition]
        );
    }

    inline void cmd_write_parition_region(size_t partition, void* data, size_t data_size, size_t data_offset) noexcept
    {
        size_t offset = std::min(
            partitions_offsets_array[partition] + data_offset, 
            partitions_offsets_array[partition] + partitions_sizes_array[partition]
        );

        size_t write_size = std::min(
            data_size,
            partitions_sizes_array[partition] - offset
        );

        pikango::cmd::write_buffer_region(
            buffer, 
            write_size,
            data, 
            offset
        );
    }
};
