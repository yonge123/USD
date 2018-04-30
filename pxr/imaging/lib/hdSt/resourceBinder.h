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
#ifndef HDST_RESOURCE_BINDER_H
#define HDST_RESOURCE_BINDER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stl.h"

#include "pxr/base/tf/hashmap.h"

PXR_NAMESPACE_OPEN_SCOPE


class HdStDrawItem;
class HdStShaderCode;
class HdStResourceGL;

typedef boost::shared_ptr<class HdStBufferResourceGL> HdStBufferResourceGLSharedPtr;
typedef boost::shared_ptr<class HdStBufferArrayRangeGL> HdStBufferArrayRangeGLSharedPtr;

typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;
typedef std::vector<HdStShaderCodeSharedPtr> HdStShaderCodeSharedPtrVector;
typedef std::vector<class HdBindingRequest> HdBindingRequestVector;

/// \class HdSt_ResourceBinder
/// 
/// A helper class to maintain all vertex/buffer/uniform binding points to be
/// used for both codegen time and rendering time.
///
/// Hydra uses 6 different types of coherent buffers.
///
/// 1. Constant buffer
///   constant primvars, which is uniform for all instances/elements/vertices.
///     ex. transform, object color
//    [SSBO, BindlessUniform]
///
/// 2. Instance buffer
///   instance primvars, one-per-instance.
///     ex. translate/scale/rotate, instanceIndices
//    [SSBO, BindlessUniform]
///
/// 3. Element buffer
///   element primvars. one-per-element (face, line).
///     ex. face color
///   [SSBO]
///
/// 4. Vertex buffer
///   vertex primvars. one-per-vertex.
///     ex. positions, normals, vertex color
///   [VertexAttribute]
///
/// 5. Index buffer
///   points/triangles/quads/lines/patches indices.
///     ex. indices, primitive param.
///   [IndexAttribute, SSBO]
///
/// 6. DrawIndex buffer
///   draw command data. one-per-drawitem (gl_DrawID equivalent)
///     ex. drawing coordinate, instance counts
///   [VertexAttribute]
///
///
///
/// For instance index indirection, three bindings are needed:
///
///    +-----------------------------------------+
///    |  instance indices buffer resource       | <-- <arrayBinding>
///    +-----------------------------------------+
///    |* culled instance indices buffer resource| <-- <culledArrayBinding>
///    +-----------------------------------------+  (bindless uniform
///                  ^   ^      ^                      or SSBO)
/// DrawCalls +---+  |   |      |
///          0|   |---   |      |
///           +---+      |      |
///          1|   |-------      |
///           +---+             |
///          2|   |--------------
///           +---+
///             ^
///             --- <baseBinding>
///                  (immediate:uniform, indirect:vertex attrib)
///
/// (*) GPU frustum culling shader shuffles instance indices into
///     culled indices buffer.
///
///
/// HdSt_ResourceBinder also takes custom bindings.
///
/// Custom bindings are used to manage bindable resources for
/// glsl shader code which is not itself generated by codegen.
///
/// For each custom binding, codegen will emit a binding definition
/// that can be used as the value of a glsl \a binding or
/// \a location layout qualifier.
///
/// e.g. Adding a custom binding of 2 for "paramsBuffer", will
/// cause codegen to emit the definition:
/// \code
/// #define paramsBuffer_Binding 2
/// \endcode
/// which can be used in a custom glsl resource declaration as:
/// \code
/// layout (binding = paramsBuffer_Binding) buffer ParamsBuffer { ... };
/// \endcode
///
class HdSt_ResourceBinder {
public:
    /// binding metadata for codegen
    class MetaData {
    public:
        MetaData() : instancerNumLevels(0) {}

        typedef size_t ID;
        /// Returns the hash value of this metadata.
        HDST_API
        ID ComputeHash() const;

        // -------------------------------------------------------------------
        // for a primvar in interleaved buffer array (Constant, ShaderData)
        struct StructEntry {
            StructEntry(TfToken const &name,
                        TfToken const &dataType,
                        int offset, int arraySize)
                : name(name)
                , dataType(dataType)
                , offset(offset)
                , arraySize(arraySize)
            {}

            TfToken name;
            TfToken dataType;
            int offset;
            int arraySize;

            bool operator < (StructEntry const &other) const {
                return offset < other.offset;
            }
        };
        struct StructBlock {
            StructBlock(TfToken const &name)
                : blockName(name) {}
            TfToken blockName;
            std::vector<StructEntry> entries;
        };
        typedef std::map<HdBinding, StructBlock> StructBlockBinding;

        // -------------------------------------------------------------------
        // for a primvar in non-interleaved buffer array (Vertex, Element, ...)
        struct Primvar {
            Primvar() {}
            Primvar(TfToken const &name, TfToken const &dataType)
                : name(name), dataType(dataType) {}
            TfToken name;
            TfToken dataType;
        };
        typedef std::map<HdBinding, Primvar> PrimvarBinding;

        // -------------------------------------------------------------------
        // for instance primvars
        struct NestedPrimvar {
            NestedPrimvar() {}
            NestedPrimvar(TfToken const &name, TfToken const &dataType,
                        int level)
                : name(name), dataType(dataType), level(level) {}
            TfToken name;
            TfToken dataType;
            int level;
        };
        typedef std::map<HdBinding, NestedPrimvar> NestedPrimvarBinding;

        // -------------------------------------------------------------------
        // for shader parameter accessors
        struct ShaderParameterAccessor {
             ShaderParameterAccessor() {}
             ShaderParameterAccessor(TfToken const &name,
                                     TfToken const &dataType,
                                     TfTokenVector const &inPrimvars=TfTokenVector())
                 : name(name), dataType(dataType), inPrimvars(inPrimvars) {}
             TfToken name;        // e.g. Kd
             TfToken dataType;    // e.g. vec4
             TfTokenVector inPrimvars;  // for primvar renaming and texture coordinates,
        };
        typedef std::map<HdBinding, ShaderParameterAccessor> ShaderParameterBinding;

        // -------------------------------------------------------------------
        // for specific buffer array (drawing coordinate, instance indices)
        struct BindingDeclaration {
            BindingDeclaration() {}
            BindingDeclaration(TfToken const &name,
                         TfToken const &dataType,
                         HdBinding binding)
                : name(name), dataType(dataType), binding(binding) {}
            TfToken name;
            TfToken dataType;
            HdBinding binding;
        };

        // -------------------------------------------------------------------

        StructBlockBinding constantData;
        StructBlockBinding shaderData;
        PrimvarBinding elementData;
        PrimvarBinding vertexData;
        PrimvarBinding fvarData;
        PrimvarBinding computeReadWriteData;
        PrimvarBinding computeReadOnlyData;
        NestedPrimvarBinding instanceData;
        int instancerNumLevels;

        ShaderParameterBinding shaderParameterBinding;

        BindingDeclaration drawingCoord0Binding;
        BindingDeclaration drawingCoord1Binding;
        BindingDeclaration drawingCoordIBinding;
        BindingDeclaration instanceIndexArrayBinding;
        BindingDeclaration culledInstanceIndexArrayBinding;
        BindingDeclaration instanceIndexBaseBinding;
        BindingDeclaration primitiveParamBinding;
        BindingDeclaration edgeIndexBinding;

        StructBlockBinding customInterleavedBindings;
        std::vector<BindingDeclaration> customBindings;
    };

    /// Constructor.
    HDST_API
    HdSt_ResourceBinder();

    /// Assign all binding points used in drawitem and custom bindings.
    /// Returns metadata to be used for codegen.
    HDST_API
    void ResolveBindings(HdStDrawItem const *drawItem,
                         HdStShaderCodeSharedPtrVector const &shaders,
                         MetaData *metaDataOut,
                         bool indirect,
                         bool instanceDraw,
                         HdBindingRequestVector const &customBindings);

    /// Assign all binding points used in computation.
    /// Returns metadata to be used for codegen.
    HDST_API
    void ResolveComputeBindings(HdBufferSpecVector const &readWriteBufferSpecs,
                                HdBufferSpecVector const &readOnlyBufferSpecs,
                                HdStShaderCodeSharedPtrVector const &shaders,
                                MetaData *metaDataOut);
    
    /// call GL introspection APIs and fix up binding locations,
    /// in case if explicit resource location qualifier is not available
    /// (GL 4.2 or before)
    HDST_API
    void IntrospectBindings(HdStResourceGL const & programResource);

    HDST_API
    void Bind(HdBindingRequest const& req) const;
    HDST_API
    void Unbind(HdBindingRequest const& req) const;

    /// bind/unbind BufferArray
    HDST_API
    void BindBufferArray(HdStBufferArrayRangeGLSharedPtr const &bar) const;
    HDST_API
    void UnbindBufferArray(HdStBufferArrayRangeGLSharedPtr const &bar) const;

    /// bind/unbind interleaved constant buffer
    HDST_API
    void BindConstantBuffer(
        HdStBufferArrayRangeGLSharedPtr const & constantBar) const;
    HDST_API
    void UnbindConstantBuffer(
        HdStBufferArrayRangeGLSharedPtr const &constantBar) const;

    /// bind/unbind nested instance BufferArray
    HDST_API
    void BindInstanceBufferArray(
        HdStBufferArrayRangeGLSharedPtr const &bar, int level) const;
    HDST_API
    void UnbindInstanceBufferArray(
        HdStBufferArrayRangeGLSharedPtr const &bar, int level) const;

    /// bind/unbind shader parameters and textures
    HDST_API
    void BindShaderResources(HdStShaderCode const *shader) const;
    HDST_API
    void UnbindShaderResources(HdStShaderCode const *shader) const;

    /// piecewise buffer binding utility
    /// (to be used for frustum culling, draw indirect result)
    HDST_API
    void BindBuffer(TfToken const &name,
                    HdStBufferResourceGLSharedPtr const &resource) const;
    HDST_API
    void BindBuffer(TfToken const &name,
                    HdStBufferResourceGLSharedPtr const &resource,
                    int offset, int level=-1) const;
    HDST_API
    void UnbindBuffer(TfToken const &name,
                      HdStBufferResourceGLSharedPtr const &resource,
                      int level=-1) const;

    /// bind(update) a standalone uniform (unsigned int)
    HDST_API
    void BindUniformui(TfToken const &name, int count,
                       const unsigned int *value) const;

    /// bind a standalone uniform (signed int, ivec2, ivec3, ivec4)
    HDST_API
    void BindUniformi(TfToken const &name, int count, const int *value) const;

    /// bind a standalone uniform array (int[N])
    HDST_API
    void BindUniformArrayi(TfToken const &name, int count, const int *value) const;

    /// bind a standalone uniform (float, vec2, vec3, vec4, mat4)
    HDST_API
    void BindUniformf(TfToken const &name, int count, const float *value) const;

    /// Returns binding point.
    /// XXX: exposed temporarily for drawIndirectResult
    /// see Hd_IndirectDrawBatch::_BeginGPUCountVisibleInstances()
    HdBinding GetBinding(TfToken const &name, int level=-1) const {
        HdBinding binding;
        TfMapLookup(_bindingMap, NameAndLevel(name, level), &binding);
        return binding;
    }

    int GetNumReservedTextureUnits() const {
        return _numReservedTextureUnits;
    }

private:
    // for batch execution
    struct NameAndLevel {
        NameAndLevel(TfToken const &n, int lv=-1) :
            name(n), level(lv) {}
        TfToken name;
        int level;

        bool operator < (NameAndLevel const &other) const {
            return name  < other.name ||
                  (name == other.name && level < other.level);
        }
    };
    typedef std::map<NameAndLevel, HdBinding> _BindingMap;
    _BindingMap _bindingMap;
    int _numReservedTextureUnits;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // HDST_RESOURCE_BINDER_H
