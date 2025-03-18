#pragma once
#include "pikango/pikango.hpp"

namespace nl
{
    struct rendertarget;
}

struct nl::rendertarget
{
private:
    bool textures_attached = false;
    pikango::texture_sized_format   color_format;

    pikango::frame_buffer_handle                frame_buffer;
    std::vector<pikango::texture_buffer_handle> color;
    pikango::texture_buffer_handle              depth_stencil;

private:
    void attach_textures()
    {
        for (int i = 0; i < color.size(); i++)
        {
            pikango::attach_to_frame_buffer(
                frame_buffer,
                color[i],
                pikango::framebuffer_attachment_type::color, 
                i
            );
        }

        pikango::attach_to_frame_buffer(
            frame_buffer, 
            depth_stencil, 
            pikango::framebuffer_attachment_type::depth, 0
        );

        pikango::attach_to_frame_buffer(
            frame_buffer, 
            depth_stencil, 
            pikango::framebuffer_attachment_type::stencil, 0
        );

        textures_attached = true;
    }

public:
    void cmd_resize(size_t width, size_t height)
    {
        for (auto& texture : color)
        {
            pikango::cmd::assign_texture_buffer_memory(
                texture,
                pikango::texture_type::texture_2d, 1, 
                color_format,
                width, height, 1
            );
        }

        pikango::cmd::assign_texture_buffer_memory(
            depth_stencil,
            pikango::texture_type::texture_2d, 1, 
            pikango::texture_sized_format::depth_24_stencil_8,
            width, height, 1
        );
    }

    rendertarget(
        pikango::command_buffer_handle& command_buffer,
        size_t                          width,
        size_t                          height,
        size_t                          color_attachments,
        pikango::texture_sized_format   color_attachments_format
    )
    {
        color_format = color_attachments_format;
        textures_attached = false;

        //Allocate resources
        frame_buffer = pikango::new_frame_buffer();

        for (int i = 0; i < color_attachments; i++)
            color.push_back(pikango::new_texture_buffer());

        depth_stencil = pikango::new_texture_buffer();
        
        //Allocate buffer memory
        pikango::begin_command_buffer_recording(command_buffer);
        cmd_resize(width, height);
        pikango::end_command_buffer_recording(command_buffer);
    }

    void cmd_bind_frame_buffer()
    {
        if (!textures_attached) attach_textures();
        pikango::cmd::bind_frame_buffer(frame_buffer);
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
