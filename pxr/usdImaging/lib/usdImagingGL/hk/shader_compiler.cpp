#include "shader_compiler.h"

#include "utils.h"
#include "exceptions.h"

#include <GlslangToSpv.h>
#include <ShaderLang.h>

namespace {
    class SPVIncluder : public glslang::TShader::Includer {
    public:
        virtual IncludeResult* include(const char* requested_source,
                                       IncludeType type,
                                       const char* requesting_source,
                                       size_t inclusion_depth)
        {
            return new IncludeResult(requested_source, "", 0, 0);
        }

        virtual void releaseInclude(IncludeResult* result)
        {
            delete result;
        }
    };

    // TODO: Read whatever is possible from the device / physical_device
    const TBuiltInResource default_builtin_resource = {
        /* .MaxLights = */ 32,
        /* .MaxClipPlanes = */ 6,
        /* .MaxTextureUnits = */ 32,
        /* .MaxTextureCoords = */ 32,
        /* .MaxVertexAttribs = */ 64,
        /* .MaxVertexUniformComponents = */ 4096,
        /* .MaxVaryingFloats = */ 64,
        /* .MaxVertexTextureImageUnits = */ 32,
        /* .MaxCombinedTextureImageUnits = */ 80,
        /* .MaxTextureImageUnits = */ 32,
        /* .MaxFragmentUniformComponents = */ 4096,
        /* .MaxDrawBuffers = */ 32,
        /* .MaxVertexUniformVectors = */ 128,
        /* .MaxVaryingVectors = */ 8,
        /* .MaxFragmentUniformVectors = */ 16,
        /* .MaxVertexOutputVectors = */ 16,
        /* .MaxFragmentInputVectors = */ 15,
        /* .MinProgramTexelOffset = */ -8,
        /* .MaxProgramTexelOffset = */ 7,
        /* .MaxClipDistances = */ 8,
        /* .MaxComputeWorkGroupCountX = */ 65535,
        /* .MaxComputeWorkGroupCountY = */ 65535,
        /* .MaxComputeWorkGroupCountZ = */ 65535,
        /* .MaxComputeWorkGroupSizeX = */ 1024,
        /* .MaxComputeWorkGroupSizeY = */ 1024,
        /* .MaxComputeWorkGroupSizeZ = */ 64,
        /* .MaxComputeUniformComponents = */ 1024,
        /* .MaxComputeTextureImageUnits = */ 16,
        /* .MaxComputeImageUniforms = */ 8,
        /* .MaxComputeAtomicCounters = */ 8,
        /* .MaxComputeAtomicCounterBuffers = */ 1,
        /* .MaxVaryingComponents = */ 60,
        /* .MaxVertexOutputComponents = */ 64,
        /* .MaxGeometryInputComponents = */ 64,
        /* .MaxGeometryOutputComponents = */ 128,
        /* .MaxFragmentInputComponents = */ 128,
        /* .MaxImageUnits = */ 8,
        /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
        /* .MaxCombinedShaderOutputResources = */ 8,
        /* .MaxImageSamples = */ 0,
        /* .MaxVertexImageUniforms = */ 0,
        /* .MaxTessControlImageUniforms = */ 0,
        /* .MaxTessEvaluationImageUniforms = */ 0,
        /* .MaxGeometryImageUniforms = */ 0,
        /* .MaxFragmentImageUniforms = */ 8,
        /* .MaxCombinedImageUniforms = */ 8,
        /* .MaxGeometryTextureImageUnits = */ 16,
        /* .MaxGeometryOutputVertices = */ 256,
        /* .MaxGeometryTotalOutputComponents = */ 1024,
        /* .MaxGeometryUniformComponents = */ 1024,
        /* .MaxGeometryVaryingComponents = */ 64,
        /* .MaxTessControlInputComponents = */ 128,
        /* .MaxTessControlOutputComponents = */ 128,
        /* .MaxTessControlTextureImageUnits = */ 16,
        /* .MaxTessControlUniformComponents = */ 1024,
        /* .MaxTessControlTotalOutputComponents = */ 4096,
        /* .MaxTessEvaluationInputComponents = */ 128,
        /* .MaxTessEvaluationOutputComponents = */ 128,
        /* .MaxTessEvaluationTextureImageUnits = */ 16,
        /* .MaxTessEvaluationUniformComponents = */ 1024,
        /* .MaxTessPatchComponents = */ 120,
        /* .MaxPatchVertices = */ 32,
        /* .MaxTessGenLevel = */ 64,
        /* .MaxViewports = */ 16,
        /* .MaxVertexAtomicCounters = */ 0,
        /* .MaxTessControlAtomicCounters = */ 0,
        /* .MaxTessEvaluationAtomicCounters = */ 0,
        /* .MaxGeometryAtomicCounters = */ 0,
        /* .MaxFragmentAtomicCounters = */ 8,
        /* .MaxCombinedAtomicCounters = */ 8,
        /* .MaxAtomicCounterBindings = */ 1,
        /* .MaxVertexAtomicCounterBuffers = */ 0,
        /* .MaxTessControlAtomicCounterBuffers = */ 0,
        /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
        /* .MaxGeometryAtomicCounterBuffers = */ 0,
        /* .MaxFragmentAtomicCounterBuffers = */ 1,
        /* .MaxCombinedAtomicCounterBuffers = */ 1,
        /* .MaxAtomicCounterBufferSize = */ 16384,
        /* .MaxTransformFeedbackBuffers = */ 4,
        /* .MaxTransformFeedbackInterleavedComponents = */ 64,
        /* .MaxCullDistances = */ 8,
        /* .MaxCombinedClipAndCullDistances = */ 8,
        /* .MaxSamples = */ 4,
        /* .limits = */ {
                               /* .nonInductiveForLoops = */ 1,
                               /* .whileLoops = */ 1,
                               /* .doWhileLoops = */ 1,
                               /* .generalUniformIndexing = */ 1,
                               /* .generalAttributeMatrixVectorIndexing = */ 1,
                               /* .generalVaryingIndexing = */ 1,
                               /* .generalSamplerIndexing = */ 1,
                               /* .generalVariableIndexing = */ 1,
                               /* .generalConstantMatrixVectorIndexing = */ 1,
                           }
    };

    EShLanguage vulkan_stage_to_glslang(vk::ShaderStageFlagBits stage)
    {
        switch (stage) {
            case vk::ShaderStageFlagBits::eVertex:
                return EShLangVertex;
            case vk::ShaderStageFlagBits::eTessellationControl:
                return EShLangTessControl;
            case vk::ShaderStageFlagBits::eTessellationEvaluation:
                return EShLangTessEvaluation;
            case vk::ShaderStageFlagBits::eGeometry:
                return EShLangGeometry;
            case vk::ShaderStageFlagBits::eFragment:
                return EShLangFragment;
            case vk::ShaderStageFlagBits::eCompute:
                return EShLangCompute;
            default:
                return EShLangCount;
        }
    }
}

namespace hk {
    std::vector<unsigned int> compile_spv(const char* shader_code, vk::ShaderStageFlagBits stage)
    {
        static SPVIncluder includer;
        const auto glslang_stage = vulkan_stage_to_glslang(stage);
        std::vector<unsigned int> spirv;
        // We have to destroy the program first according to ShaderLang.h
        auto shader = make_unique<glslang::TShader>(glslang_stage);
        auto program = make_unique<glslang::TProgram>();
        shader->setStrings(&shader_code, 1);
        EShMessages options = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
        if (!shader->parse(&default_builtin_resource, 450, ENoProfile, false, false, options, includer)) {
            throw vulkan_exception(std::string("Error parsing shader!\n") + std::string(shader->getInfoLog()));
        }

        program->addShader(shader.get());
        if (!program->link(options)) {
            throw vulkan_exception(std::string("Error linking program!\n") + std::string(program->getInfoLog()));
        }
        glslang::GlslangToSpv(*program->getIntermediate(glslang_stage), spirv, nullptr);
        return spirv;
    }

    void initialize_compiler() {
        ShInitialize();
    }

    void finalize_compiler() {
        ShFinalize();
    }
}
