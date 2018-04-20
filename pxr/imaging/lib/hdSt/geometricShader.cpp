//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/geometricShader.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/debugCodes.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/glf/glslfx.h"

#include <boost/functional/hash.hpp>

#include <string>

PXR_NAMESPACE_OPEN_SCOPE


HdSt_GeometricShader::HdSt_GeometricShader(std::string const &glslfxString,
                                       PrimitiveType primType,
                                       HdCullStyle cullStyle,
                                       HdPolygonMode polygonMode,
                                       bool cullingPass,
                                       SdfPath const &debugId,
                                       float lineWidth)
    : HdStShaderCode()
    , _primType(primType)
    , _cullStyle(cullStyle)
    , _polygonMode(polygonMode)
    , _lineWidth(lineWidth)
    , _cullingPass(cullingPass)
    , _hash(0)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    // XXX
    // we will likely move this (the constructor or the entire class) into
    // the base class (HdStShaderCode) at the end of refactoring, to be able to
    // use same machinery other than geometric shaders.

    if (TfDebug::IsEnabled(HD_DUMP_GLSLFX_CONFIG)) {
        std::cout << debugId << "\n"
                  << glslfxString << "\n";
    }

    std::stringstream ss(glslfxString);
    _glslfx.reset(new GlfGLSLFX(ss));
    boost::hash_combine(_hash, _glslfx->GetHash());
    boost::hash_combine(_hash, cullingPass);
    boost::hash_combine(_hash, primType);
    //
    // note: Don't include cullStyle and polygonMode into the hash.
    //      They are independent from the GLSL program.
    //
}

HdSt_GeometricShader::~HdSt_GeometricShader()
{
    // nothing
}

/* virtual */
HdStShaderCode::ID
HdSt_GeometricShader::ComputeHash() const
{
    return _hash;
}

/* virtual */
std::string
HdSt_GeometricShader::GetSource(TfToken const &shaderStageKey) const
{
    return _glslfx->GetSource(shaderStageKey);
}

const HdBufferArrayRangeSharedPtr&
HdSt_GeometricShader::GetShaderData() const
{
    return _paramArray;
}

void
HdSt_GeometricShader::BindResources(HdSt_ResourceBinder const &binder, int program)
{
    if (_cullStyle != HdCullStyleDontCare) {
        unsigned int cullStyle = _cullStyle;
        binder.BindUniformui(HdShaderTokens->cullStyle, 1, &cullStyle);
    } else {
        // don't care -- use renderPass's fallback
    }

    if (GetPrimitiveMode() == GL_PATCHES) {
        glPatchParameteri(GL_PATCH_VERTICES, GetPrimitiveIndexSize());
    }

    if (_polygonMode == HdPolygonModeLine) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if (_lineWidth > 0) {
            glLineWidth(_lineWidth);
        }
    }

    if (!_textureDescriptors.empty()) {
        auto samplerUnit = binder.GetNumReservedTextureUnits();
        for (const auto& it: _textureDescriptors) {
            auto binding = binder.GetBinding(it.name);
            if (binding.GetType() == HdBinding::TEXTURE_2D) {
                glActiveTexture(GL_TEXTURE0 + samplerUnit);
                glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(it.handle));
                glBindSampler(samplerUnit, it.sampler);
                glProgramUniform1i(program, binding.GetLocation(), samplerUnit);
                samplerUnit++;
            }
        }

        glActiveTexture(GL_TEXTURE0);
        binder.BindShaderResources(this);
    }
}

void
HdSt_GeometricShader::UnbindResources(HdSt_ResourceBinder const &binder, int program)
{
    if (_polygonMode == HdPolygonModeLine) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    if (!_textureDescriptors.empty()) {
        binder.UnbindShaderResources(this);

        auto samplerUnit = binder.GetNumReservedTextureUnits();
        for (const auto& it: _textureDescriptors) {
            auto binding = binder.GetBinding(it.name);
            if (binding.GetType() == HdBinding::TEXTURE_2D) {
                glActiveTexture(GL_TEXTURE0 + samplerUnit);
                glBindTexture(GL_TEXTURE_2D, 0);
                glBindSampler(samplerUnit, 0);
                samplerUnit++;
            }
        }

        glActiveTexture(GL_TEXTURE0);
    }
}

/*virtual*/
void
HdSt_GeometricShader::AddBindings(HdBindingRequestVector *customBindings)
{
    // no-op
}

GLenum
HdSt_GeometricShader::GetPrimitiveMode() const 
{
    GLenum primMode = GL_POINTS;

    switch (_primType)
    {
        case PrimitiveType::PRIM_POINTS:
            primMode = GL_POINTS;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_LINES:
            primMode = GL_LINES;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
            primMode = GL_TRIANGLES;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case PrimitiveType::PRIM_MESH_REFINED_QUADS:
            primMode = GL_LINES_ADJACENCY;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES:
        case PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES:
        case PrimitiveType::PRIM_MESH_PATCHES:
            primMode = GL_PATCHES;
            break;    
    }

    return primMode;
}

int
HdSt_GeometricShader::GetPrimitiveIndexSize() const
{
    int primIndexSize = 1;

    switch (_primType)
    {
        case PrimitiveType::PRIM_POINTS:
            primIndexSize = 1;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_LINES:
        case PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES:
            primIndexSize = 2;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
            primIndexSize = 3;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES:
        case PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case PrimitiveType::PRIM_MESH_REFINED_QUADS:
            primIndexSize = 4;
            break;
        case PrimitiveType::PRIM_MESH_PATCHES:
            primIndexSize = 16;
            break;
    }

    return primIndexSize;
}

int
HdSt_GeometricShader::GetNumPrimitiveVertsForGeometryShader() const
{
    int numPrimVerts = 1;

    switch (_primType)
    {
        case PrimitiveType::PRIM_POINTS:
            numPrimVerts = 1;
            break;
        case PrimitiveType::PRIM_BASIS_CURVES_LINES:
            numPrimVerts = 2;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_TRIANGLES:
        case PrimitiveType::PRIM_MESH_REFINED_TRIANGLES:
        case PrimitiveType::PRIM_BASIS_CURVES_LINEAR_PATCHES:
        case PrimitiveType::PRIM_BASIS_CURVES_CUBIC_PATCHES:
        case PrimitiveType::PRIM_MESH_PATCHES: 
        // for patches with tesselation, input to GS is still a series of tris
            numPrimVerts = 3;
            break;
        case PrimitiveType::PRIM_MESH_COARSE_QUADS:
        case PrimitiveType::PRIM_MESH_REFINED_QUADS:
            numPrimVerts = 4;
            break;
    }

    return numPrimVerts;
}

void
HdSt_GeometricShader::SetTextureDescriptors(const TextureDescriptorVector &texDesc)
{
    _textureDescriptors = texDesc;
}

void
HdSt_GeometricShader::SetBufferSources(
    HdBufferSourceVector& bufferSources,
    const HdResourceRegistrySharedPtr& resourceRegistry) {
    if (bufferSources.empty()) {
        _paramArray.reset();
    } else {
        // Build the buffer Spec to see if its changed.
        HdBufferSpecVector bufferSpecs;
        TF_FOR_ALL(srcIt, bufferSources) {
            (*srcIt)->AddBufferSpecs(&bufferSpecs);
        }

        if (!_paramArray || _paramSpec != bufferSpecs) {
            _paramSpec = bufferSpecs;

            // establish a buffer range
            HdBufferArrayRangeSharedPtr range =
                resourceRegistry->AllocateShaderStorageBufferArrayRange(
                    HdTokens->materialParams,
                    bufferSpecs);

            if (!TF_VERIFY(range->IsValid())) {
                _paramArray.reset();
            } else {
                _paramArray = range;
            }
        }

        if (_paramArray->IsValid()) {
            resourceRegistry->AddSources(_paramArray, bufferSources);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

