#include "hkEngine.h"

#include "hk/vulkan_includer.h"
#include "hk/utils.h"
#include "hk/exceptions.h"
#include "hk/buffer.h"
#include "hk/shader_compiler.h"
#include "hk/instance.h"
#include "hk/device.h"
#include "hk/shape.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/gf/matrix4f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usd/attribute.h"

// USD Geom schemas
#include "pxr/usd/usdGeom/tokens.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdGeom/mesh.h"

// Hydra lights
#include "pxr/usd/usdHydra/distantLight.h"
#include "pxr/usd/usdHydra/sphereLight.h"

#include <typeinfo>

// Note, while exceptions are generally frowned upon we are not afraid of using them here.
// Most of the compilers are able to optimize exception usage, so when nothing is thrown
// there is no overhead. And when something is thrown, we are failing everything anyway
// so exceptions are caught maximum once.

namespace {
    const char* simple_vertex_code = R"glsl(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 in_pos;

layout (binding = 0) uniform WorldBlock
{
    mat4 view_matrix;
	mat4 projection_matrix;
};

layout (push_constant) uniform PushConstants {
    mat4 model_matrix;
} push_constants;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
	gl_Position = projection_matrix * view_matrix * push_constants.model_matrix * vec4(in_pos.xyz, 1.0);
}
    )glsl";

    const char* simple_fragment_code = R"glsl(
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(1.0, 1.0, 1.0, 1.0);
}
    )glsl";
}

struct UsdImagingGLHkEngine::Impl {
    using queue_families_ordering_t = std::vector<std::pair<uint32_t, vk::QueueFamilyProperties>>;

    Impl(const SdfPathVector& _excluded_paths) : is_valid(false),
                                                 viewport(
                                                     -std::numeric_limits<float>::max(),
                                                     -std::numeric_limits<float>::max(),
                                                     -std::numeric_limits<float>::max(),
                                                     -std::numeric_limits<float>::max()
                                                 ), camera_changed(false), viewport_changed(false),
                                                 excluded_paths(_excluded_paths)
    {
        hk::initialize_compiler();
        std::sort(excluded_paths.begin(), excluded_paths.end()); // so lower bound will work
        const bool enable_validation = TfGetenv("HK_ENABLE_VALIDATION", "0") == "1";
        if (enable_validation) {
            TF_WARN("[hk] Validation layers are enabled, if you see this message in production, contact the closest developer!");
        }
        try {
            instance = hk::create_instance(enable_validation);
            physical_device = hk::get_sorted_physical_devices(instance).front();
            physical_device_properties = physical_device.getProperties();
            physical_device_features = physical_device.getFeatures();
            physical_device_memory_properties = physical_device.getMemoryProperties();

            // We have to find the queue family, that supports the most features we want,
            // and offers the most queues
            // One trick, we also have to track the queue id, or else we won't know which of the queues
            // query from the device (that works based on position from the official vector)
            // The ordering policy is quire simple, we look at the number of queues supported first
            // then having the most variety in supported features.
            auto device_queue_family_properties = physical_device.getQueueFamilyProperties();
            if (device_queue_family_properties.size() == 0) {
                throw hk::vulkan_exception_empty_vector(MARK_ERROR("No queue families found for the current device!"));
            }
            queue_families_ordering_t queue_family_properties_ordered;
            queue_family_properties_ordered.reserve(device_queue_family_properties.size());
            int queue_family_counter = 0;
            for (const auto& queue_family : device_queue_family_properties) {
                queue_family_properties_ordered.emplace_back(queue_family_counter++, queue_family);
            }
            std::sort(queue_family_properties_ordered.begin(), queue_family_properties_ordered.end(),
                      [](const queue_families_ordering_t::value_type& a,
                         const queue_families_ordering_t::value_type& b) {
                          if (a.second.queueCount > b.second.queueCount)
                              return true;
                          else if ((static_cast<unsigned int>(a.second.queueFlags) & 0x0000000F) >
                                   (static_cast<unsigned int>(b.second.queueFlags) & 0x0000000F))
                              return true;
                          else
                              return a.first < b.first;
                      });
            // Then we find the first queue, that supports graphics, compute and transfer at the same time, because
            // we require that.
            // TODO: check the specification if this is required.
            selected_queue_familiy_index = std::numeric_limits<uint32_t>::max();
            for (const auto& queue_family : queue_family_properties_ordered) {
                if (queue_family.second.queueFlags & vk::QueueFlagBits::eGraphics &&
                    queue_family.second.queueFlags & vk::QueueFlagBits::eCompute &&
                    queue_family.second.queueFlags & vk::QueueFlagBits::eTransfer) {
                    queue_family_properties = queue_family.second;
                    selected_queue_familiy_index = queue_family.first;
                    break;
                }
            }
            if (selected_queue_familiy_index == std::numeric_limits<uint32_t>::max()) {
                throw hk::vulkan_exception{MARK_ERROR("Couldn't find a queue that fits our requirements!")};
            }

            // We are just creating one queue for now
            float queue_priorities[] = {1.0f};
            vk::DeviceQueueCreateInfo device_queue_info{
                vk::DeviceQueueCreateFlags(), selected_queue_familiy_index, 1, queue_priorities
            };
            std::vector<const char*> required_physical_device_extensions;
            // required_physical_device_extensions.push_back(VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME);
            auto available_physical_device_extensions = physical_device.enumerateDeviceExtensionProperties(nullptr).value;
            if (!hk::check_for_extensions(required_physical_device_extensions, available_physical_device_extensions)) {
                throw hk::vulkan_exception("The required extensions are not available for the physical device!");
            }
            device = hk::unwrap(physical_device.createDevice(
                {
                    vk::DeviceCreateFlags(),
                    1, &device_queue_info,
                    static_cast<uint32_t>(required_physical_device_extensions.size()),
                    required_physical_device_extensions.data(),
                }), MARK_ERROR("Error creating device!"));
            queue = device.getQueue(selected_queue_familiy_index, 0);
            command_pool = hk::unwrap(device.createCommandPool(
                {
                    vk::CommandPoolCreateFlagBits::eResetCommandBuffer
                }), MARK_ERROR("Error creating command pool!"));
            copy_command_buffer = hk::unwrap_vector(device.allocateCommandBuffers(
                { command_pool, vk::CommandBufferLevel::ePrimary, 1 }),
                MARK_ERROR("Error allocating command buffer!")).front();
            copy_fence = hk::unwrap(device.createFence({}), MARK_ERROR("Error creating frence!"));
            device.resetFences({copy_fence});
            draw_command_buffer = hk::unwrap_vector(device.allocateCommandBuffers(
                { command_pool, vk::CommandBufferLevel::ePrimary, 1 }),
                MARK_ERROR("Error allocating command buffer!")).front();
            camera_buffer = {
                device, physical_device_memory_properties,
                sizeof(CameraParams),
                vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
                vk::MemoryPropertyFlagBits::eDeviceLocal};
            camera_staging_buffer = {
                device, physical_device_memory_properties,
                camera_buffer
            };
            // Just setting up some descriptors atm
            std::array<vk::DescriptorPoolSize, 1> pool_sizes = {
                vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1024)
            };
            uint32_t total_pool_size = 0;
            for (const auto& pool_size : pool_sizes) {
                total_pool_size += pool_size.descriptorCount;
            }
            descriptor_pool = hk::unwrap(device.createDescriptorPool(
                {
                    vk::DescriptorPoolCreateFlags(), // for now we don't want to return descriptor sets
                    total_pool_size, static_cast<uint32_t>(pool_sizes.size()), pool_sizes.data()
                }), MARK_ERROR("Error creating descriptor pool!"));
            vk::DescriptorSetLayoutBinding set_binding {
                0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr
            };
            camera_buffer_descriptor_set_layout = hk::unwrap(device.createDescriptorSetLayout(
                {
                    vk::DescriptorSetLayoutCreateFlags(),
                    1, &set_binding,
                }), MARK_ERROR("Error creating descriptor set layout!"));
            camera_buffer_descriptor_set = hk::unwrap_vector(device.allocateDescriptorSets(
                {
                    descriptor_pool, 1, &camera_buffer_descriptor_set_layout
                }), MARK_ERROR("Error allocating camera buffer descriptor set!")).front();
            vk::DescriptorBufferInfo camera_buffer_info {
                camera_buffer.buffer, 0, VK_WHOLE_SIZE
            };
            vk::WriteDescriptorSet descriptor_set_write {
                camera_buffer_descriptor_set, 0, 0, 1, vk::DescriptorType::eUniformBuffer,
                nullptr, &camera_buffer_info, nullptr
            };
            device.updateDescriptorSets(
                1, &descriptor_set_write,
                0, nullptr);
            const auto vertex_shader_spv = hk::compile_spv(simple_vertex_code, vk::ShaderStageFlagBits::eVertex);
            const auto fragment_shader_spv = hk::compile_spv(simple_fragment_code, vk::ShaderStageFlagBits::eFragment);
            vertex_module = hk::unwrap(device.createShaderModule(
                {
                    vk::ShaderModuleCreateFlags(),
                    hk::vector_bsize_u32(vertex_shader_spv), vertex_shader_spv.data()
                }), MARK_ERROR("Error creating vertex shader module!"));
            fragment_module = hk::unwrap(device.createShaderModule(
                {
                    vk::ShaderModuleCreateFlags(),
                    hk::vector_bsize_u32(fragment_shader_spv), fragment_shader_spv.data()
                }), MARK_ERROR("Error creating fragment shader module!"));

            vk::PushConstantRange push_constant_range = {
                vk::ShaderStageFlagBits::eVertex, 0, sizeof(GfMatrix4f)
            };
            pipeline_layout = hk::unwrap(device.createPipelineLayout(
                {
                    vk::PipelineLayoutCreateFlags(),
                    1, &camera_buffer_descriptor_set_layout,
                    1, &push_constant_range
                }), MARK_ERROR("Error creating pipeline layout!"));

            // Setting up the render pass
            std::array<vk::AttachmentDescription, 2> attachment_descriptions{
                vk::AttachmentDescription(
                    vk::AttachmentDescriptionFlags(),
                    vk::Format::eR8G8B8A8Unorm, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal
                ),
                vk::AttachmentDescription(
                    vk::AttachmentDescriptionFlags(),
                    vk::Format::eD32Sfloat, vk::SampleCountFlagBits::e1,
                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                    vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal
                )
            };
            vk::AttachmentReference rgba_attachment_reference {
                0, vk::ImageLayout::eColorAttachmentOptimal
            };
            vk::AttachmentReference depth_attachment_reference {
                1, vk::ImageLayout::eDepthStencilAttachmentOptimal
            };
            vk::SubpassDescription subpass_description {
                vk::SubpassDescriptionFlags(),
                vk::PipelineBindPoint::eGraphics,
                0, nullptr,
                1, &rgba_attachment_reference,
                nullptr, &depth_attachment_reference,
                0, nullptr
            };
            render_pass = hk::unwrap(device.createRenderPass(
                {
                    vk::RenderPassCreateFlags(),
                    2, attachment_descriptions.data(),
                    1, &subpass_description,
                    0, nullptr
                }), MARK_ERROR("Error creating render pass!"));

            params.frame = -std::numeric_limits<double>::max(); // so update will be triggered on the first frame
            is_valid = true;
        } catch (hk::vulkan_exception_error_code& e) {
            TF_WARN("[hk] %s (%s)", e.description.c_str(), vk::to_string(e.error_code).c_str());
        } catch (hk::vulkan_exception_empty_vector& e) {
            TF_WARN("[hk] %s (%s)", e.description.c_str(), "Returned empty vector!");
        } catch (hk::vulkan_exception& e) {
            TF_WARN("[hk] %s", e.description.c_str());
        }
    }

    ~Impl()
    {
        hk::finalize_compiler();
        try { // we are caching unimplemented clear resource exceptions
            if (device) {
                for (auto& shape : shapes) {
                    shape.release(device);
                }
                clear_resource(copy_fence);
                camera_buffer.release(device);
                camera_staging_buffer.release(device);
                clear_resource(command_pool);
                clear_resource(vertex_module);
                clear_resource(fragment_module);
                clear_resource(camera_buffer_descriptor_set_layout);
                clear_resource(descriptor_pool);
                clear_resource(pipeline_layout);
                clear_framebuffers();
                clear_resource(render_pass);
                clear_resource(pipeline);
                device.destroy();
            }
            if (instance) {
                instance.destroy();
            }
        } catch(const hk::vulkan_exception& e) {
            std::cerr << e.description << std::endl;
            throw e;
        }
    }

    template <typename T>
    void clear_resource(T& resource) {
        hk::device_clear_resource(device, resource);
    }

    void clear_framebuffers() {
        clear_resource(rgba_image);
        clear_resource(depth_image);
        clear_resource(rgba_view);
        clear_resource(depth_view);
        clear_resource(rgba_memory);
        clear_resource(depth_memory);
        clear_resource(frame_buffer);
    }

    void setup_framebuffers() {
        if (!viewport_changed) {
            return;
        }
        viewport_changed = false;

        if (device) {
            clear_framebuffers();
            clear_resource(pipeline);
        }

        const auto width = viewport[2] - viewport[0];
        const auto height = viewport[3] - viewport[1];

        rgba_image = hk::unwrap(device.createImage(
            {
                vk::ImageCreateFlags(),
                vk::ImageType::e2D, vk::Format::eR8G8B8A8Unorm,
                {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
                1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined
            }), MARK_ERROR("Error making rgba image!"));
        depth_image = hk::unwrap(device.createImage(
            {
                vk::ImageCreateFlags(),
                vk::ImageType::e2D, vk::Format::eD32Sfloat,
                {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
                1, 1, vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                vk::SharingMode::eExclusive, 0, nullptr, vk::ImageLayout::eUndefined
            }), MARK_ERROR("Error making depth image!"));
        auto rgba_reqs = device.getImageMemoryRequirements(rgba_image);
        auto depth_reqs = device.getImageMemoryRequirements(depth_image);
        rgba_memory = hk::unwrap(device.allocateMemory(
            {
                rgba_reqs.size,
                hk::get_memory_type(physical_device_memory_properties,
                                    rgba_reqs.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal)
            }), MARK_ERROR("Error allocating rgba memory!"));
        depth_memory = hk::unwrap(device.allocateMemory(
            {
                depth_reqs.size,
                hk::get_memory_type(physical_device_memory_properties,
                                    depth_reqs.memoryTypeBits,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal)
            }), MARK_ERROR("Error allocating depth memory!"));
        hk::unwrap_void(device.bindImageMemory(rgba_image, rgba_memory, 0),
                        MARK_ERROR("Error binding rgba image to memory!"));
        hk::unwrap_void(device.bindImageMemory(depth_image, depth_memory, 0),
                        MARK_ERROR("Error binding depth image to memory!"));
        rgba_view = hk::unwrap(device.createImageView(
            {
                vk::ImageViewCreateFlags(),
                rgba_image, vk::ImageViewType::e2D, vk::Format::eR8G8B8A8Unorm,
                {
                    vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                    vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA
                }, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
            }), MARK_ERROR("Error creating rgba view!"));
        depth_view = hk::unwrap(device.createImageView(
            {
                vk::ImageViewCreateFlags(),
                depth_image, vk::ImageViewType::e2D, vk::Format::eD32Sfloat,
                {
                    vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                    vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA
                }, {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
            }), MARK_ERROR("Error creating depth view!"));
        std::array<vk::ImageView, 2> attachments = {
            rgba_view, depth_view
        };
        frame_buffer = hk::unwrap(device.createFramebuffer(
            {
                vk::FramebufferCreateFlags(),
                render_pass,
                static_cast<uint32_t>(attachments.size()), attachments.data(),
                static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1
            }), MARK_ERROR("Error creating frame buffer!"));

        std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages = {
            vk::PipelineShaderStageCreateInfo(
                vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vertex_module, "main"
            ),
            vk::PipelineShaderStageCreateInfo(
                vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, fragment_module, "main"
            )
        };
        vk::GraphicsPipelineCreateInfo pipeline_info = {};
        pipeline_info.stageCount = static_cast<uint32_t>(shader_stages.size());
        pipeline_info.pStages = shader_stages.data();
        pipeline_info.layout = pipeline_layout;
    }

    void render(const UsdPrim& _root, RenderParams _params) {
        setup_framebuffers();
        // First step is to make sure all the data is copied
        // for that right now we are using staging buffers, even for UBO, even though there are other approaches
        hk::unwrap_void(copy_command_buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit}),
                        MARK_NONE());
        if (camera_changed) {
            camera_changed = false;
            camera_staging_buffer.write_from_host(device, camera_params);
            camera_staging_buffer.copy_to(copy_command_buffer, camera_buffer);
        }

        bool scene_changed = hk::overwrite_if_different(root, _root);
        scene_changed |= hk::overwrite_if_different(params.frame, _params.frame);

        static std::vector<hk::SimpleStagingBuffer> staging_buffers;
        staging_buffers.clear();

        for (auto& shape : shapes) {
           clear_resource(shape);
        }
        shapes.clear();

        GfMatrix4d current_matrix(1.0);
        // Reload the scene for now
        // TODO: install notifiers to handle changes properly
        if (scene_changed) {
            params = _params;
            std::vector<std::pair<UsdPrim, GfMatrix4d>> xform_stack;
            const auto pseudo_root = root.GetStage()->GetPseudoRoot();
            for (auto prim_iter = UsdTreeIterator::PreAndPostVisit(_root); prim_iter; ++prim_iter) {
                if (not prim_iter.IsPostVisit()) {
                    if (hk::is_in_sorted_vector(excluded_paths, prim_iter->GetPath())) {
                        prim_iter.PruneChildren();
                        continue;
                    }

                    const auto prim = *prim_iter;

                    bool visible = true;
                    TfToken visibility;
                    if (prim != pseudo_root &&
                        prim_iter->GetAttribute(UsdGeomTokens->visibility).Get(&visibility, params.frame) &&
                        visibility == UsdGeomTokens->invisible) {
                        visible = false;
                    }

                    if (visible) {
                        TfToken purpose;
                        if (prim != pseudo_root &&
                            prim_iter->GetAttribute(UsdGeomTokens->purpose).Get(&purpose, params.frame) &&
                            (purpose == UsdGeomTokens->guide && !_params.showGuides)) {
                            visible = false;
                        }
                    }

                    auto handle_xform = [&current_matrix, &xform_stack, &prim] (const UsdTimeCode& frame) {
                        GfMatrix4d xform(1.0);
                        UsdGeomXform xform_prim(prim);
                        bool reset_xform_stack = false;
                        xform_prim.GetLocalTransformation(&xform, &reset_xform_stack, frame);
                        static const GfMatrix4d IDENTITY(1.0);
                        if (xform != IDENTITY) {
                            xform_stack.push_back(std::make_pair(prim, current_matrix));
                            if (not reset_xform_stack) {
                                current_matrix = xform * current_matrix;
                            }
                            else {
                                current_matrix = xform;
                            }
                        }
                    };

                    if (visible) {
                        if (prim.IsA<UsdGeomXform>()) {
                            handle_xform(params.frame);
                        } else if (prim.IsA<UsdGeomMesh>()) {
                            handle_xform(params.frame);
                            UsdGeomMesh geo(prim);

                            VtVec3fArray vertices;
                            VtIntArray vertex_counts;
                            VtIntArray vertex_indices;
                            geo.GetPointsAttr().Get(&vertices, params.frame);
                            geo.GetFaceVertexCountsAttr().Get(&vertex_counts, params.frame);
                            geo.GetFaceVertexIndicesAttr().Get(&vertex_indices, params.frame);

                            if (vertices.size() < 1 &&
                                vertex_counts.size() < 1 &&
                                vertex_indices.size() < 3) {
                                continue;
                            }

                            static std::vector<uint32_t> indices;
                            indices.clear();
                            const size_t guessed_index_count = 3 * vertex_counts.size() / 2;
                            if (indices.capacity() < guessed_index_count) {
                                indices.reserve(guessed_index_count);
                            }

                            auto vertex_index_ptr = vertex_indices.data();
                            for (auto vertex_count : vertex_counts) {
                                if (vertex_count < 3) {
                                    if (vertex_count > 0)
                                        vertex_index_ptr += vertex_count;
                                    continue;
                                }

                                const auto id0 = static_cast<uint32_t>(std::max(*vertex_index_ptr, 0));
                                for (int ii = 2; ii < vertex_count; ++ii) {
                                    indices.push_back(id0);
                                    indices.push_back(static_cast<uint32_t>(std::max(*(vertex_index_ptr + ii - 1), 0)));
                                    indices.push_back(static_cast<uint32_t>(std::max(*(vertex_index_ptr + ii), 0)));
                                }
                                vertex_index_ptr += vertex_count;
                            }

                            if (indices.size() < 3) {
                                continue;
                            }

                            hk::SimpleShape shape;
                            struct Vertex {
                                GfVec3f vertex;
                            };

                            shape.object_matrix = GfMatrix4f(current_matrix);
                            shape.vertex_buffer = {
                                device, physical_device_memory_properties,
                                static_cast<uint32_t>(vertices.size() * sizeof(GfVec3f)),
                                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                                vk::MemoryPropertyFlagBits::eDeviceLocal
                            };
                            staging_buffers.emplace_back(
                                device, physical_device_memory_properties, shape.vertex_buffer,
                                copy_command_buffer, vertices.data()
                            );
                            shape.index_buffer = {
                                device, physical_device_memory_properties,
                                hk::vector_bsize_u32(indices),
                                vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                                vk::MemoryPropertyFlagBits::eDeviceLocal
                            };
                            staging_buffers.emplace_back(
                                device, physical_device_memory_properties, shape.index_buffer,
                                copy_command_buffer, indices.data()
                            );

                            shapes.push_back(shape);
                        }
                    } else {
                        prim_iter.PruneChildren();
                    }
                } else {
                    if (not xform_stack.empty()) {
                        const auto& last_elem = xform_stack.back();
                        if (last_elem.first == *prim_iter) {
                            xform_stack.pop_back();
                            current_matrix = last_elem.second;
                        }
                    }
                }
            }
        }

        copy_command_buffer.end();
        // TODO, change the copy fence to a semaphore
        const vk::PipelineStageFlags wait_state_flags = vk::PipelineStageFlagBits::eAllGraphics;
        hk::unwrap_void(queue.submit(
            {{
                 0, nullptr, // wait semaphores
                 &wait_state_flags, 1, &copy_command_buffer,
                 0, nullptr // signal semaphores
             }}, copy_fence), MARK_NONE());

        device.waitForFences({copy_fence}, 1, UINT64_MAX);
        device.resetFences({copy_fence});

        for (auto& staging_buffer : staging_buffers) {
            clear_resource(staging_buffer);
        }
        staging_buffers.clear();
    }

    void set_camera_state(const GfMatrix4f& viewMatrix,
                          const GfMatrix4f& projectionMatrix,
                          const GfVec4f& _viewport) {
        camera_changed = hk::overwrite_if_different(camera_params.view_matrix, viewMatrix);
        camera_changed |= hk::overwrite_if_different(camera_params.projection_matrix, projectionMatrix);
        viewport_changed = hk::overwrite_if_different(viewport, _viewport);
    }

    void invalidate_buffers() {

    }

    // Frequently accessed data
    vk::Instance instance;
    vk::Device device;
    vk::Queue queue;
    vk::CommandPool command_pool;
    vk::CommandBuffer draw_command_buffer;
    vk::Pipeline pipeline;
    vk::DescriptorSet camera_buffer_descriptor_set;
    bool is_valid;

    // Somewhat frequently accessed data
    RenderParams params;
    std::vector<hk::SimpleShape> shapes;
    vk::PhysicalDeviceMemoryProperties physical_device_memory_properties;
    struct CameraParams {
        GfMatrix4f view_matrix;
        GfMatrix4f projection_matrix;
    } camera_params;
    GfVec4f viewport;
    hk::SimpleBuffer camera_buffer;
    hk::SimpleStagingBuffer camera_staging_buffer;
    vk::CommandBuffer copy_command_buffer;
    vk::Fence copy_fence;
    UsdPrim root;
    bool camera_changed;
    bool viewport_changed;

    // Less frequently accessedd data
    SdfPathVector excluded_paths;
    vk::PhysicalDevice physical_device;
    vk::PhysicalDeviceProperties physical_device_properties;
    vk::PhysicalDeviceFeatures physical_device_features;
    vk::QueueFamilyProperties queue_family_properties;
    vk::ShaderModule vertex_module;
    vk::ShaderModule fragment_module;
    vk::PipelineLayout pipeline_layout;
    vk::DescriptorSetLayout camera_buffer_descriptor_set_layout;
    vk::DescriptorPool descriptor_pool;
    vk::RenderPass render_pass;
    vk::Image rgba_image;
    vk::Image depth_image;
    vk::ImageView rgba_view;
    vk::ImageView depth_view;
    vk::DeviceMemory rgba_memory;
    vk::DeviceMemory depth_memory;
    vk::Framebuffer frame_buffer;
    uint32_t selected_queue_familiy_index;
};

UsdImagingGLHkEngine::UsdImagingGLHkEngine(const SdfPathVector& excluded_paths)
    : pimpl(hk::make_unique<UsdImagingGLHkEngine::Impl>(excluded_paths)) {

}

UsdImagingGLHkEngine::~UsdImagingGLHkEngine() {

}

void UsdImagingGLHkEngine::Render(const UsdPrim& root, RenderParams params) {
    if (pimpl->is_valid) {
        pimpl->is_valid = false;
        try {
            pimpl->render(root, params);
            pimpl->is_valid = true;
        } catch (hk::vulkan_exception_error_code& e) {
            TF_WARN("[hk] %s (%s)", e.description.c_str(), vk::to_string(e.error_code).c_str());
        } catch (hk::vulkan_exception_empty_vector& e) {
            TF_WARN("[hk] %s (%s)", e.description.c_str(), "Returned empty vector!");
        } catch (hk::vulkan_exception& e) {
            TF_WARN("[hk] %s", e.description.c_str());
        }
    }
};

void UsdImagingGLHkEngine::SetCameraState(const GfMatrix4d& viewMatrix,
                    const GfMatrix4d& projectionMatrix,
                    const GfVec4d& viewport) {
    pimpl->set_camera_state(
        GfMatrix4f(viewMatrix),
        GfMatrix4f(projectionMatrix),
        GfVec4f(viewport)
    );
}

void UsdImagingGLHkEngine::SetLightingState(GlfSimpleLightVector const &lights,
                      GlfSimpleMaterial const &material,
                      GfVec4f const &sceneAmbient) {

}

void UsdImagingGLHkEngine::InvalidateBuffers() {
    pimpl->invalidate_buffers();
};
