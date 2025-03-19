#pragma once
#include "pikango/pikango.hpp"

namespace nl
{
    struct rendertarget;
}

struct nl::rendertarget
{
private:
    size_t                          current_width;
    size_t                          current_height;
    size_t                          color_attachments_amount;
    pikango::texture_sized_format   color_format;

    pikango::frame_buffer_handle                frame_buffer;
    std::vector<pikango::texture_buffer_handle> color;
    pikango::texture_buffer_handle              depth_stencil;

private:
    void create_attachments()
    {
        pikango::texture_buffer_create_info tbci;

        tbci.type           = pikango::texture_type::texture_2d;
        tbci.mipmap_layers  = 1;
        tbci.memory_format  = color_format;
        tbci.dim1           = current_width;
        tbci.dim2           = current_height;
        tbci.dim3           = 1;  

        for (int i = 0; i < color_attachments_amount; i++)
        {
            auto attachment = pikango::new_texture_buffer(tbci);
            color.push_back(attachment);

            pikango::attach_to_frame_buffer(
                frame_buffer, 
                attachment, 
                pikango::framebuffer_attachment_type::color, 
                i
            );
        }

        tbci.memory_format = pikango::texture_sized_format::depth_24_stencil_8;

        depth_stencil = pikango::new_texture_buffer(tbci);

        pikango::attach_to_frame_buffer(
            frame_buffer,
            depth_stencil,
            pikango::framebuffer_attachment_type::depth,
            0
        );

        pikango::attach_to_frame_buffer(
            frame_buffer,
            depth_stencil,
            pikango::framebuffer_attachment_type::stencil,
            0
        );
    }

public:
    void resize(size_t width, size_t height)
    {
        if (width == current_width && height == current_height) return;

        color.clear();
        depth_stencil.~handle();

        create_attachments();
    }

    rendertarget(
        size_t                          width,
        size_t                          height,
        size_t                          color_attachments,
        pikango::texture_sized_format   color_attachments_format
    )
    {
        current_width  = width;
        current_height = height;
        
        color_attachments_amount = color_attachments;
        color_format             = color_attachments_format;

        frame_buffer = pikango::new_frame_buffer(pikango::frame_buffer_create_info{});
        create_attachments();
    }

    pikango::frame_buffer_handle get_frame_buffer()
    {
        return frame_buffer;
    }

    pikango::texture_buffer_handle get_color_texture(size_t index)
    {
        if (index >= color.size()) return {};
        return color[index];
    }

    pikango::texture_buffer_handle get_depth_stencil_texture()
    {
        return depth_stencil;
    }
};
