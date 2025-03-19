#pragma once
#include "pikango/pikango.hpp"

#include <list>
#include <set>
#include <vector>
#include <unordered_map>

namespace nl
{
    class paged_buffer;
}

class nl::paged_buffer
{
private:
    struct page_info
    {
        size_t                  size;
        pikango::buffer_handle  buffer;
    };

    struct partition_info
    {
        std::list<page_info>::iterator   page;
        size_t                           size;
        size_t                           offset;
    };

    struct partitions_by_size
    {
        bool operator()(const partition_info& a, const partition_info& b) const
        {
            return a.size < b.size;
        }
    };
    
    struct partitions_by_location
    {
        bool operator()(const partition_info& a, const partition_info& b) const
        {
            if (a.page == b.page)
                return a.offset < b.offset;
            return &(*a.page) < &(*b.page);
        }

        bool operator()(const std::multiset<partition_info>::iterator& a, const std::multiset<partition_info>::iterator& b) const
        {
            return partitions_by_location()(*a, *b);
        }
    };

private:
    size_t default_page_size;
    size_t partition_size_alignment;

    pikango::buffer_memory_profile  pages_memory_profile;
    pikango::buffer_access_profile  pages_access_profile;

    std::list<page_info> pages;

    size_t partitions_names_gen = 0;
    std::unordered_map<size_t, partition_info> allocated_partitions;

    std::multiset<partition_info, partitions_by_size>                         free_partitions;
    std::set<std::multiset<partition_info>::iterator, partitions_by_location> free_partitions_by_offset;

private:
    inline size_t align_partition_size(size_t size)
    {
        auto mod = (size % partition_size_alignment);
        if (mod) return size + partition_size_alignment - mod;
        else     return size;
    }

    std::multiset<partition_info>::iterator push_free_partition(partition_info partition)
    {
        auto itr = free_partitions.insert(partition);
        free_partitions_by_offset.insert(itr);
        return itr;
    }

    std::multiset<partition_info>::iterator erase_free_partition(std::multiset<partition_info>::iterator partition)
    {
        free_partitions_by_offset.erase(partition);
        return free_partitions.erase(partition);
    }

    void create_page(size_t size)
    {
        pikango::buffer_create_info bci;
        bci.buffer_size_bytes = size;
        bci.memory_profile = pages_memory_profile;
        bci.access_profile = pages_access_profile;

        auto buffer = pikango::new_buffer(bci);

        page_info page;

        page.size = size;
        page.buffer = buffer;

        pages.push_back(page);
        auto page_itr = (--pages.end());

        partition_info info;
        info.offset = 0;
        info.size = size;
        info.page = page_itr;

        push_free_partition(info);
    }

    void erase_page(std::list<page_info>::iterator page)
    {
        auto itr = free_partitions.begin();

        while (itr != free_partitions.end())
        {
            if (itr->page == page)
                itr = erase_free_partition(itr);
            else
                itr++;
        }

        pages.erase(page);
    }

    std::multiset<partition_info>::iterator merge_neighbour_free_partitions(std::multiset<partition_info>::iterator free_partition)
    {
        auto free_partition_by_offset = free_partitions_by_offset.find(free_partition);
        
        auto left_partition_itr_itr  = free_partition_by_offset; left_partition_itr_itr--;
        auto right_partition_itr_itr = free_partition_by_offset; right_partition_itr_itr++;

        auto new_free_partition = *free_partition;

        if (
            free_partition_by_offset != free_partitions_by_offset.begin() &&    //if partition is begin, then partition to the left does not exist
            (*left_partition_itr_itr)->page == new_free_partition.page    &&    //on same page (the page object pointer is the same)
            (*left_partition_itr_itr)->offset + (*left_partition_itr_itr)->size == new_free_partition.offset //partitions are neighbours (are not separated)
        )
        {
            new_free_partition.offset = (*left_partition_itr_itr)->offset;
            new_free_partition.size  += (*left_partition_itr_itr)->size;

            erase_free_partition(*left_partition_itr_itr);
        }

        if (
            right_partition_itr_itr != free_partitions_by_offset.end() &&                               //iterator not empty
            &(*(*right_partition_itr_itr)->page) == &(*new_free_partition.page) &&                      //on same page (the page object pointer is the same)
            new_free_partition.offset + new_free_partition.size == (*right_partition_itr_itr)->offset   //partitions are neighbours (are not separated)
        )
        {
            new_free_partition.size  += (*right_partition_itr_itr)->size;

            erase_free_partition(*right_partition_itr_itr);
        }

        erase_free_partition(free_partition);
        return push_free_partition(new_free_partition);
    }

public:
    paged_buffer(
        size_t                          page_size,
        size_t                          partition_alignment,
        pikango::buffer_memory_profile  memory_profile,
        pikango::buffer_access_profile  access_profile
    )
    {   
        default_page_size = page_size;
        partition_size_alignment = partition_alignment;

        pages_memory_profile = memory_profile;
        pages_access_profile = access_profile;

        //create one page, for starters
        create_page(default_page_size);
    }

    inline pikango::buffer_handle get_partition_buffer(size_t partition) noexcept
    {
        auto itr = allocated_partitions.find(partition);
        if (itr == allocated_partitions.end()) return {};
        return itr->second.page->buffer;
    }

    inline const size_t get_partition_size(size_t partition) const noexcept
    {
        auto itr = allocated_partitions.find(partition);
        if (itr == allocated_partitions.end()) return 0;
        return itr->second.size;
    }

    inline const size_t get_partition_offset(size_t partition) const noexcept
    {
        auto itr = allocated_partitions.find(partition);
        if (itr == allocated_partitions.end()) return 0;
        return itr->second.offset;
    }

    inline size_t allocate_partition(size_t partition_size) noexcept
    {
        partition_size = align_partition_size(partition_size);

        partition_info comp;
        comp.size = partition_size;

        auto itr = free_partitions.lower_bound(comp);
        if (itr == free_partitions.end())
        {
            create_page(std::max(partition_size, default_page_size));
            return allocate_partition(partition_size);
        }

        auto free_partition = *itr; 
        erase_free_partition(itr);

        partition_info allocated;
        allocated.offset = free_partition.offset;
        allocated.size   = partition_size;
        allocated.page   = free_partition.page;

        if (partition_size < free_partition.size)
        {
            free_partition.offset = allocated.offset + allocated.size;
            free_partition.size   = free_partition.size - partition_size;
            
            push_free_partition(free_partition);
        }
        
        size_t name = partitions_names_gen++;
        allocated_partitions.insert({name, std::move(allocated)});
        return name;
    }

    inline void free_partition(size_t partition) noexcept
    {
        auto itr = allocated_partitions.find(partition);
        if (itr == allocated_partitions.end()) return;

        auto info = itr->second;
        allocated_partitions.erase(itr);

        auto free_partition = push_free_partition(info);
        free_partition = merge_neighbour_free_partitions(free_partition);

        //if page is all empty erase
        if (
            free_partition->size == free_partition->page->size && 
            pages.size() != 1
        )
            erase_page(free_partition->page);  
    }

    inline void cmd_write_parition(size_t partition, void* data) noexcept
    {
        auto itr = allocated_partitions.find(partition);
        if (itr == allocated_partitions.end()) return;
        
        auto& info = itr->second;

        pikango::cmd::write_buffer_region(
            info.page->buffer,
            info.size,
            data,
            info.offset
        );
    }

    inline void cmd_write_parition_region(size_t partition, void* data, size_t data_size, size_t data_offset) noexcept
    {
        auto itr = allocated_partitions.find(partition);
        if (itr == allocated_partitions.end()) return;
        
        auto& info = itr->second;

        size_t offset = info.offset + std::min(data_offset, info.size);
        size_t size = std::min(data_size, info.size - offset);

        pikango::cmd::write_buffer_region(
            info.page->buffer,
            size,
            data,
            offset
        );
    }
};
