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
#include "usdMaya/util.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <maya/MAnimUtil.h>
#include <maya/MColor.h>
#include <maya/MDGModifier.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnExpression.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnSet.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MObject.h>
#include <maya/MPlugArray.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTime.h>


#include <string>
#include <unordered_map>


// return seconds per frame
double PxrUsdMayaUtil::spf()
{
    static const MTime sec(1.0, MTime::kSeconds);
    return 1.0 / sec.as(MTime::uiUnit());
}

MStatus
PxrUsdMayaUtil::GetMObjectByName(const std::string& nodeName, MObject& mObj)
{
    MSelectionList selectionList;
    MStatus status = selectionList.add(MString(nodeName.c_str()));
    if (status != MS::kSuccess) {
        return status;
    }

    status = selectionList.getDependNode(0, mObj);

    return status;
}

MStatus
PxrUsdMayaUtil::GetDagPathByName(const std::string& nodeName, MDagPath& dagPath)
{
    MSelectionList selectionList;
    MStatus status = selectionList.add(MString(nodeName.c_str()));
    if (status != MS::kSuccess) {
        return status;
    }

    status = selectionList.getDagPath(0, dagPath);

    return status;
}

bool PxrUsdMayaUtil::isAncestorDescendentRelationship(const MDagPath & path1,
    const MDagPath & path2)
{
    unsigned int length1 = path1.length();
    unsigned int length2 = path2.length();
    unsigned int diff;

    if (length1 == length2 && !(path1 == path2))
        return false; 
    MDagPath ancestor, descendent;
    if (length1 > length2)
    {
        ancestor = path2;
        descendent = path1;
        diff = length1 - length2;
    }
    else
    {
        ancestor = path1;
        descendent = path2;
        diff = length2 - length1;
    }

    descendent.pop(diff);

    bool ret = (ancestor == descendent);

    if (ret)
    {
        MString err = path1.fullPathName() + " and ";
        err += path2.fullPathName() + " have parenting relationships";
        MGlobal::displayError(err);
    }
    return ret;
}



// returns 0 if static, 1 if sampled, and 2 if a curve
int PxrUsdMayaUtil::getSampledType(const MPlug& iPlug, bool includeConnectedChildren)
{
    MPlugArray conns;

    iPlug.connectedTo(conns, true, false);

    // it's possible that only some element of an array plug or
    // some component of a compound plus is connected
    if (conns.length() == 0)
    {
        if (iPlug.isArray())
        {
            unsigned int numConnectedElements = iPlug.numConnectedElements();
            for (unsigned int e = 0; e < numConnectedElements; e++)
            {
                // For now we assume that when you encounter an array of plugs we want always to include connected children
                int retVal = getSampledType(iPlug.connectionByPhysicalIndex(e), true);
                if (retVal > 0)
                    return retVal;
            }
        }
        else if (iPlug.isCompound() && iPlug.numConnectedChildren() > 0
                && includeConnectedChildren)
        {
            unsigned int numChildren = iPlug.numChildren();
            for (unsigned int c = 0; c < numChildren; c++)
            {
                int retVal = getSampledType(iPlug.child(c), true);
                if (retVal > 0)
                    return retVal;
            }
        }
        return 0;
    }

    MObject ob;
    MFnDependencyNode nodeFn;
    for (unsigned i = 0; i < conns.length(); i++)
    {
        ob = conns[i].node();
        MFn::Type type = ob.apiType();

        switch (type)
        {
            case MFn::kAnimCurveTimeToAngular:
            case MFn::kAnimCurveTimeToDistance:
            case MFn::kAnimCurveTimeToTime:
            case MFn::kAnimCurveTimeToUnitless:
            {
                nodeFn.setObject(ob);
                MPlug incoming = nodeFn.findPlug("i", true);

                // sampled
                if (incoming.isConnected())
                    return 1;

                // curve
                else
                    return 2;
            }
            break;

            case MFn::kMute:
            {
                nodeFn.setObject(ob);
                MPlug mutePlug = nodeFn.findPlug("mute", true);

                // static
                if (mutePlug.asBool())
                    return 0;
                // curve
                else
                   return 2;
            }
            break;

            default:
            break;
        }
    }

    return 1;
}

bool PxrUsdMayaUtil::getRotOrder(MTransformationMatrix::RotationOrder iOrder,
    unsigned int & oXAxis, unsigned int & oYAxis, unsigned int & oZAxis)
{
    switch (iOrder)
    {
        case MTransformationMatrix::kXYZ:
        {
            oXAxis = 0;
            oYAxis = 1;
            oZAxis = 2;
        }
        break;

        case MTransformationMatrix::kYZX:
        {
            oXAxis = 1;
            oYAxis = 2;
            oZAxis = 0;
        }
        break;

        case MTransformationMatrix::kZXY:
        {
            oXAxis = 2;
            oYAxis = 0;
            oZAxis = 1;
        }
        break;

        case MTransformationMatrix::kXZY:
        {
            oXAxis = 0;
            oYAxis = 2;
            oZAxis = 1;
        }
        break;

        case MTransformationMatrix::kYXZ:
        {
            oXAxis = 1;
            oYAxis = 0;
            oZAxis = 2;
        }
        break;

        case MTransformationMatrix::kZYX:
        {
            oXAxis = 2;
            oYAxis = 1;
            oZAxis = 0;
        }
        break;

        default:
        {
            return false;
        }
    }
    return true;
}

// 0 dont write, 1 write static 0, 2 write anim 0, 3 write anim -1
int PxrUsdMayaUtil::getVisibilityType(const MPlug & iPlug)
{
    int type = getSampledType(iPlug, true);

    // static case
    if (type == 0)
    {
        // dont write anything
        if (iPlug.asBool())
            return 0;

        // write static 0
        return 1;
    }
    else
    {
        // anim write -1
        if (iPlug.asBool())
            return 3;

        // write anim 0
        return 2;
    }
}

// does this cover all cases?
bool PxrUsdMayaUtil::isAnimated(MObject & object, bool checkParent)
{
    MStatus stat;
    MItDependencyGraph iter(object, MFn::kInvalid,
        MItDependencyGraph::kUpstream,
        MItDependencyGraph::kDepthFirst,
        MItDependencyGraph::kNodeLevel,
        &stat);

    if (stat!= MS::kSuccess)
    {
        MGlobal::displayError("Unable to create DG iterator ");
    }

    // MAnimUtil::isAnimated(node) will search the history of the node
    // for any animation curve nodes. It will return true for those nodes
    // that have animation curve in their history.
    // The average time complexity is O(n^2) where n is the number of history
    // nodes. But we can improve the best case by split the loop into two.
    std::vector<MObject> nodesToCheckAnimCurve;

    for (; !iter.isDone(); iter.next())
    {
        MObject node = iter.thisNode();

        if (node.hasFn(MFn::kPluginDependNode) ||
                node.hasFn( MFn::kConstraint ) ||
                node.hasFn(MFn::kPointConstraint) ||
                node.hasFn(MFn::kAimConstraint) ||
                node.hasFn(MFn::kOrientConstraint) ||
                node.hasFn(MFn::kScaleConstraint) ||
                node.hasFn(MFn::kGeometryConstraint) ||
                node.hasFn(MFn::kNormalConstraint) ||
                node.hasFn(MFn::kTangentConstraint) ||
                node.hasFn(MFn::kParentConstraint) ||
                node.hasFn(MFn::kPoleVectorConstraint) ||
                node.hasFn(MFn::kParentConstraint) ||
                node.hasFn(MFn::kTime) ||
                node.hasFn(MFn::kJoint) ||
                node.hasFn(MFn::kGeometryFilt) ||
                node.hasFn(MFn::kTweak) ||
                node.hasFn(MFn::kPolyTweak) ||
                node.hasFn(MFn::kSubdTweak) ||
                node.hasFn(MFn::kCluster) ||
                node.hasFn(MFn::kFluid) ||
                node.hasFn(MFn::kPolyBoolOp))
        {
            return true;
        }

        if (node.hasFn(MFn::kExpression))
        {
            MFnExpression fn(node, &stat);
            if (stat == MS::kSuccess && fn.isAnimated())
            {
                return true;
            }
        }

        nodesToCheckAnimCurve.push_back(node);
    }

    for (size_t i = 0; i < nodesToCheckAnimCurve.size(); i++)
    {
        if (MAnimUtil::isAnimated(nodesToCheckAnimCurve[i], checkParent))
        {
            return true;
        }
    }

    return false;
}

bool PxrUsdMayaUtil::isIntermediate(const MObject & object)
{
    MStatus stat;
    MFnDagNode mFn(object);

    MPlug plug = mFn.findPlug("intermediateObject", false, &stat);
    if (stat == MS::kSuccess && plug.asBool())
        return true;
    else
        return false;
}

bool PxrUsdMayaUtil::isRenderable(const MObject & object)
{
    MStatus stat;
    MFnDagNode mFn(object);

    // templated turned on?  return false
    MPlug plug = mFn.findPlug("template", false, &stat);
    if (stat == MS::kSuccess && plug.asBool())
        return false;

    // visibility or lodVisibility off?  return false
    plug = mFn.findPlug("visibility", false, &stat);
    if (stat == MS::kSuccess && !plug.asBool())
    {
        // the value is off. let's check if it has any in-connection,
        // otherwise, it means it is not animated.
        MPlugArray arrayIn;
        plug.connectedTo(arrayIn, true, false, &stat);

        if (stat == MS::kSuccess && arrayIn.length() == 0)
        {
            return false;
        }
    }

    plug = mFn.findPlug("lodVisibility", false, &stat);
    if (stat == MS::kSuccess && !plug.asBool())
    {
        MPlugArray arrayIn;
        plug.connectedTo(arrayIn, true, false, &stat);

        if (stat == MS::kSuccess && arrayIn.length() == 0)
        {
            return false;
        }
    }

    // this shape is renderable
    return true;
}

MString PxrUsdMayaUtil::stripNamespaces(const MString & iNodeName, unsigned int iDepth)
{
    if (iDepth == 0)
    {
        return iNodeName;
    }

    MStringArray strArray;
    if (iNodeName.split(':', strArray) == MS::kSuccess)
    {
        unsigned int len = strArray.length();

        // we want to strip off more namespaces than what we have
        // so we just return the last name
        if (len == 0)
        {
            return iNodeName;
        }
        else if (len <= iDepth + 1)
        {
            return strArray[len-1];
        }

        MString name;
        for (unsigned int i = iDepth; i < len - 1; ++i)
        {
            name += strArray[i];
            name += ":";
        }
        name += strArray[len-1];
        return name;
    }

    return iNodeName;
}

std::string PxrUsdMayaUtil::SanitizeName(const std::string& name)
{
    return TfStringReplace(name, ":", "_");
}

// This to allow various pipeline to sanitize the colorset name for output
std::string PxrUsdMayaUtil::SanitizeColorSetName(const std::string& name)
{
    //We sanitize the name since certain pipeline like Pixar's, we have rman_ in front of all color sets that need to be exportred. We now export all colosets
    size_t namePos=0;
    static const std::string RMAN_PREFIX("rman_");
    if (name.find(RMAN_PREFIX) == 0)
        namePos=RMAN_PREFIX.size();
    return name.substr(namePos);
}

// Get array (constant or per component) of attached shaders
// Pass a non-zero value for numComponents when retrieving shaders on an object
// that supports per-component shader assignment (e.g. faces of a polymesh).
// In this case, shaderObjs will be the length of the number of shaders
// assigned to the object. assignmentIndices will be the length of
// numComponents, with values indexing into shaderObjs.
// When numComponents is zero, shaderObjs will be of length 1 and
// assignmentIndices will be empty.
static
bool
_getAttachedMayaShaderObjects(
        const MFnDagNode &node,
        const unsigned int numComponents,
        MObjectArray *shaderObjs,
        VtArray<int> *assignmentIndices)
{
    bool hasShader=false;
    MStatus status;

    // This structure maps shader object names to their indices in the
    // shaderObjs array. We use this to make sure that we add each unique
    // shader to shaderObjs only once.
    TfHashMap<std::string, size_t, TfHash> shaderPlugsMap;

    shaderObjs->clear();
    assignmentIndices->clear();

    MObjectArray setObjs;
    MObjectArray compObjs;
    node.getConnectedSetsAndMembers(0, setObjs, compObjs, true); // Assuming that not using instancing

    // If we have multiple components and either multiple sets or one set with
    // only a subset of the object in it, we'll keep track of the assignments
    // for all components in assignmentIndices. We initialize all of the
    // assignments as unassigned using a value of -1.
    if (numComponents > 1 && 
            (setObjs.length() > 1 || 
             (setObjs.length() == 1 && !compObjs[0].isNull()))) {
        assignmentIndices->assign((size_t)numComponents, -1);
    }

    for (unsigned int i=0; i < setObjs.length(); ++i) {
        // Get associated Set and Shading Group
        MFnSet setFn(setObjs[i], &status);
        MPlug seSurfaceShaderPlg = setFn.findPlug("surfaceShader", &status);

        // Find connection shader->shadingGroup
        MPlugArray plgCons;
        seSurfaceShaderPlg.connectedTo(plgCons, true, false, &status);
        if (plgCons.length() == 0) {
            continue;
        }

        hasShader = true;
        MPlug shaderPlug = plgCons[0];
        MObject shaderObj = shaderPlug.node();

        auto inserted = shaderPlugsMap.insert(
            std::pair<std::string, size_t>(shaderPlug.name().asChar(),
                                           shaderObjs->length()));
        if (inserted.second) {
            shaderObjs->append(shaderObj);
        }

        // If we are tracking per-component assignments, mark all components of
        // this set as assigned to this shader.
        if (!assignmentIndices->empty()) {
            size_t shaderIndex = inserted.first->second;

            MItMeshPolygon faceIt(node.dagPath(), compObjs[i]);
            for (faceIt.reset(); !faceIt.isDone(); faceIt.next()) {
                (*assignmentIndices)[faceIt.index()] = shaderIndex;
            }
        }
    }

    return hasShader;
}

bool
_GetColorAndTransparencyFromLambert(
        const MObject& shaderObj,
        GfVec3f* rgb,
        float* alpha)
{
    MStatus status;
    MFnLambertShader lambertFn(shaderObj, &status );
    if (status == MS::kSuccess ) {
        if (rgb) {
            GfVec3f displayColor;
            MColor color = lambertFn.color();
            for (int j=0;j<3;j++) {
                displayColor[j] = color[j];
            }
            *rgb = GfConvertDisplayToLinear(displayColor);
        }
        if (alpha) {
            MColor trn = lambertFn.transparency();
            // Assign Alpha as 1.0 - average of shader trasparency 
            // and check if they are all the same
            *alpha = 1.0 - ((trn[0] + trn[1] + trn[2]) / 3.0);
        }
        return true;
    } 
    
    return false;
}

bool
_GetColorAndTransparencyFromDepNode(
        const MObject& shaderObj,
        GfVec3f* rgb,
        float* alpha)
{
    MStatus status;
    MFnDependencyNode d(shaderObj);
    MPlug colorPlug = d.findPlug("color", &status);
    if (!status) {
        return false;
    }
    MPlug transparencyPlug = d.findPlug("transparency", &status);
    if (!status) {
        return false;
    }

    if (rgb) {
        GfVec3f displayColor;
        for (int j=0; j<3; j++) {
            colorPlug.child(j).getValue(displayColor[j]);
        }
        *rgb = GfConvertDisplayToLinear(displayColor);
    }

    if (alpha) {
        float trans = 0.f;
        for (int j=0; j<3; j++) {
            float t = 0.f;
            transparencyPlug.child(j).getValue(t);
            trans += t/3.f;
        }
        (*alpha) = 1.f - trans;
    }
    return true;
}

static void 
_getMayaShadersColor(
        const MObjectArray &shaderObjs, 
        VtArray<GfVec3f> *RGBData,
        VtArray<float> *AlphaData)
{
    MStatus status;

    if (shaderObjs.length() == 0) {
        return;
    }

    if (RGBData) {
        RGBData->resize(shaderObjs.length());
    }
    if (AlphaData) {
        AlphaData->resize(shaderObjs.length());
    }

    for (unsigned int i = 0; i < shaderObjs.length(); ++i) {
        // Initialize RGB and Alpha to (1,1,1,1)
        if (RGBData) {
            (*RGBData)[i][0] = 1.0;
            (*RGBData)[i][1] = 1.0;
            (*RGBData)[i][2] = 1.0;
        }
        if (AlphaData) {
            (*AlphaData)[i] = 1.0;
        }

        if (shaderObjs[i].isNull()) {
            MGlobal::displayError("Invalid Maya Shader Object at index: " +
                                  MString(TfStringPrintf("%d", i).c_str()) +
                                  ". Unable to retrieve ShaderBaseColor.");
            continue;
        }

        // first, we assume the shader is a lambert and try that API.  if
        // not, we try our next best guess.
        bool gotValues = _GetColorAndTransparencyFromLambert(
                shaderObjs[i],
                RGBData ?  &(*RGBData)[i] : NULL,
                AlphaData ?  &(*AlphaData)[i] : NULL)

            || _GetColorAndTransparencyFromDepNode(
                shaderObjs[i],
                RGBData ?  &(*RGBData)[i] : NULL,
                AlphaData ?  &(*AlphaData)[i] : NULL);

        if (!gotValues) {
            MGlobal::displayError("Failed to get shaders colors at index: " +
                                  MString(TfStringPrintf("%d", i).c_str()) +
                                  ". Unable to retrieve ShaderBaseColor.");
        }
    }
}

static
bool
_GetLinearShaderColor(
        const MFnDagNode& node,
        const unsigned int numComponents,
        VtArray<GfVec3f> *RGBData,
        VtArray<float> *AlphaData,
        TfToken *interpolation,
        VtArray<int> *assignmentIndices)
{
    MObjectArray shaderObjs;
    if (_getAttachedMayaShaderObjects(node, numComponents, &shaderObjs, assignmentIndices)) {
        if (assignmentIndices && interpolation) {
            if (assignmentIndices->empty()) {
                *interpolation = UsdGeomTokens->constant;
            } else {
                *interpolation = UsdGeomTokens->uniform;
            }
        }

        _getMayaShadersColor(shaderObjs, RGBData, AlphaData);

        return true;
    }

    return false;
}

bool
PxrUsdMayaUtil::GetLinearShaderColor(
        const MFnDagNode& node,
        VtArray<GfVec3f> *RGBData,
        VtArray<float> *AlphaData,
        TfToken *interpolation,
        VtArray<int> *assignmentIndices)
{
    return _GetLinearShaderColor(node,
                                 0,
                                 RGBData,
                                 AlphaData,
                                 interpolation,
                                 assignmentIndices);
}

bool
PxrUsdMayaUtil::GetLinearShaderColor(
        const MFnMesh& mesh,
        VtArray<GfVec3f> *RGBData,
        VtArray<float> *AlphaData,
        TfToken *interpolation,
        VtArray<int> *assignmentIndices)
{
    unsigned int numComponents = mesh.numPolygons();
    return _GetLinearShaderColor(mesh,
                                 numComponents,
                                 RGBData,
                                 AlphaData,
                                 interpolation,
                                 assignmentIndices);
}

template <typename T>
struct ValueHash
{
    std::size_t operator() (const T& value) const {
        return hash_value(value);
    }
};

template <typename T>
struct ValuesEqual
{
    bool operator() (const T& a, const T& b) const {
        return GfIsClose(a, b, 1e-9);
    }
};

template <typename T>
static
void
_MergeEquivalentIndexedValues(
        VtArray<T>* valueData,
        VtArray<int>* assignmentIndices)
{
    if (!valueData || !assignmentIndices) {
        return;
    }

    const size_t numValues = valueData->size();
    if (numValues == 0) {
        return;
    }

    // We maintain a map of values to that value's index in our uniqueValues
    // array.
    std::unordered_map<T, size_t, ValueHash<T>, ValuesEqual<T> > valuesMap;
    VtArray<T> uniqueValues;
    VtArray<int> uniqueIndices;

    for (size_t i = 0; i < assignmentIndices->size(); ++i) {
        int index = (*assignmentIndices)[i];

        if (index < 0 || static_cast<size_t>(index) >= numValues) {
            // This is an unassigned or otherwise unknown index, so just keep it.
            uniqueIndices.push_back(index);
            continue;
        }

        const T value = (*valueData)[index];

        int uniqueIndex = -1;

        auto inserted = valuesMap.insert(
            std::pair<T, size_t>(value, uniqueValues.size()));
        if (inserted.second) {
            // This is a new value, so add it to the array.
            uniqueValues.push_back(value);
            uniqueIndex = uniqueValues.size() - 1;
        } else {
            // This is an existing value, so re-use the original's index.
            uniqueIndex = inserted.first->second;
        }

        uniqueIndices.push_back(uniqueIndex);
    }

    // If we reduced the number of values by merging, copy the results back.
    if (uniqueValues.size() < numValues) {
        (*valueData) = uniqueValues;
        (*assignmentIndices) = uniqueIndices;
    }
}

void
PxrUsdMayaUtil::MergeEquivalentIndexedValues(
        VtArray<float>* valueData,
        VtArray<int>* assignmentIndices) {
    return _MergeEquivalentIndexedValues<float>(valueData, assignmentIndices);
}

void
PxrUsdMayaUtil::MergeEquivalentIndexedValues(
        VtArray<GfVec2f>* valueData,
        VtArray<int>* assignmentIndices) {
    return _MergeEquivalentIndexedValues<GfVec2f>(valueData, assignmentIndices);
}

void
PxrUsdMayaUtil::MergeEquivalentIndexedValues(
        VtArray<GfVec3f>* valueData,
        VtArray<int>* assignmentIndices) {
    return _MergeEquivalentIndexedValues<GfVec3f>(valueData, assignmentIndices);
}

void
PxrUsdMayaUtil::MergeEquivalentIndexedValues(
        VtArray<GfVec4f>* valueData,
        VtArray<int>* assignmentIndices) {
    return _MergeEquivalentIndexedValues<GfVec4f>(valueData, assignmentIndices);
}

void
PxrUsdMayaUtil::CompressFaceVaryingPrimvarIndices(
        const MFnMesh& mesh,
        TfToken *interpolation,
        VtArray<int>* assignmentIndices)
{
    if (!interpolation     || 
        !assignmentIndices || 
         assignmentIndices->size() == 0) {
        return;
    }

    // Use -2 as the initial "un-stored" sentinel value, since -1 is the
    // default unauthored value index for primvars.
    int numPolygons = mesh.numPolygons();
    VtArray<int> uniformAssignments;
    uniformAssignments.assign((size_t)numPolygons, -2);

    int numVertices = mesh.numVertices();
    VtArray<int> vertexAssignments;
    vertexAssignments.assign((size_t)numVertices, -2);

    // We assume that the data is constant/uniform/vertex until we can
    // prove otherwise that two components have differing values.
    bool isConstant = true;
    bool isUniform = true;
    bool isVertex = true;

    MItMeshFaceVertex itFV(mesh.object());
    unsigned int fvi = 0;
    for (itFV.reset(); !itFV.isDone(); itFV.next(), ++fvi) {
        int faceIndex = itFV.faceId();
        int vertexIndex = itFV.vertId();

        int assignedIndex = (*assignmentIndices)[fvi];

        if (isConstant) {
            if (assignedIndex != (*assignmentIndices)[0]) {
                isConstant = false;
            }
        }

        if (isUniform) {
            if (uniformAssignments[faceIndex] < -1) {
                // No value for this face yet, so store one.
                uniformAssignments[faceIndex] = assignedIndex;
            } else if (assignedIndex != uniformAssignments[faceIndex]) {
                isUniform = false;
            }
        }

        if (isVertex) {
            if (vertexAssignments[vertexIndex] < -1) {
                // No value for this vertex yet, so store one.
                vertexAssignments[vertexIndex] = assignedIndex;
            } else if (assignedIndex != vertexAssignments[vertexIndex]) {
                isVertex = false;
            }
        }

        if (!isConstant && !isUniform && !isVertex) {
            // No compression will be possible, so stop trying.
            break;
        }
    }

    if (isConstant) {
        assignmentIndices->resize(1);
        *interpolation = UsdGeomTokens->constant;
    } else if (isUniform) {
        *assignmentIndices = uniformAssignments;
        *interpolation = UsdGeomTokens->uniform;
    } else if(isVertex) {
        *assignmentIndices = vertexAssignments;
        *interpolation = UsdGeomTokens->vertex;
    } else {
        *interpolation = UsdGeomTokens->faceVarying;
    }
}

bool
PxrUsdMayaUtil::AddUnassignedUVIfNeeded(
        VtArray<GfVec2f>* uvData,
        VtArray<int>* assignmentIndices,
        int* unassignedValueIndex,
        const GfVec2f& defaultUV)
{
    if (!assignmentIndices || assignmentIndices->empty()) {
        return false;
    }

    *unassignedValueIndex = -1;

    for (size_t i = 0; i < assignmentIndices->size(); ++i) {
        if ((*assignmentIndices)[i] >= 0) {
            // This component has an assignment, so skip it.
            continue;
        }

        // We found an unassigned index. Add the unassigned value to uvData
        // if we haven't already.
        if (*unassignedValueIndex < 0) {
            if (uvData) {
                uvData->push_back(defaultUV);
            }
            *unassignedValueIndex = uvData->size() - 1;
        }

        // Assign the component the unassigned value index.
        (*assignmentIndices)[i] = *unassignedValueIndex;
    }

    return true;
}

bool
PxrUsdMayaUtil::AddUnassignedColorAndAlphaIfNeeded(
        VtArray<GfVec3f>* RGBData,
        VtArray<float>* AlphaData,
        VtArray<int>* assignmentIndices,
        int* unassignedValueIndex,
        const GfVec3f& defaultRGB,
        const float defaultAlpha)
{
    if (!assignmentIndices || assignmentIndices->empty()) {
        return false;
    }

    if (RGBData && AlphaData && (RGBData->size() != AlphaData->size())) {
        TF_CODING_ERROR("Unequal sizes for color (%zu) and opacity (%zu)",
                        RGBData->size(), AlphaData->size());
    }

    *unassignedValueIndex = -1;

    for (size_t i=0; i < assignmentIndices->size(); ++i) {
        if ((*assignmentIndices)[i] >= 0) {
            // This component has an assignment, so skip it.
            continue;
        }

        // We found an unassigned index. Add unassigned values to RGBData and
        // AlphaData if we haven't already.
        if (*unassignedValueIndex < 0) {
            if (RGBData) {
                RGBData->push_back(defaultRGB);
            }
            if (AlphaData) {
                AlphaData->push_back(defaultAlpha);
            }
            *unassignedValueIndex = RGBData->size() - 1;
        }

        // Assign the component the unassigned value index.
        (*assignmentIndices)[i] = *unassignedValueIndex;
    }

    return true;
}

MPlug
PxrUsdMayaUtil::GetConnected(const MPlug& plug)
{
    MStatus status = MS::kFailure;
    MPlugArray conn;
    plug.connectedTo(conn, true, false, &status);
    if (!status || conn.length() != 1) {
        return MPlug();
    }
    return conn[0];
}

void
PxrUsdMayaUtil::Connect(
        const MPlug& srcPlug,
        const MPlug& dstPlug,
        bool clearDstPlug)
{
    MStatus status;
    MDGModifier dgMod;

    if (clearDstPlug) {
        MPlugArray plgCons;
        dstPlug.connectedTo(plgCons, true, false, &status);
        for (unsigned int i=0; i < plgCons.length(); ++i) {
            status = dgMod.disconnect(plgCons[i], dstPlug);
        }
    }

    // Execute the disconnect/connect
    status = dgMod.connect(srcPlug, dstPlug);
    dgMod.doIt();
}

// XXX: see logic in MayaTransformWriter.  It's unfortunate that this
// logic is in 2 places.  we should merge.
static bool
_IsShape(const MDagPath& dagPath) {
    if (dagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    // go to the parent
    MDagPath parentDagPath = dagPath;
    parentDagPath.pop();
    if (!parentDagPath.hasFn(MFn::kTransform)) {
        return false;
    }

    unsigned int numberOfShapesDirectlyBelow = 0;
    parentDagPath.numberOfShapesDirectlyBelow(numberOfShapesDirectlyBelow);
    return (numberOfShapesDirectlyBelow == 1);
}

MObject
PxrUsdMayaUtil::GetReferenceNode(const MString& referencedNodeName)
{
    MStatus status;

    // Don't know how to query a node's reference using OpenMaya
    MString cmd("referenceQuery -referenceNode \"");
    cmd += referencedNodeName;
    cmd += "\";";
    MString refNodeName = MGlobal::executeCommandStringResult(
            cmd, false, false, &status);
    // If the node is NOT from a ref - ie, '|persp' - then the command will error.
    // This command is supposed to "check" if a node is referenced, so this is
    // "normal" behavior, so don't use CHECK_MSTATUS
    if (!status || refNodeName.length() == 0) return MObject::kNullObj;

    MObject mObj;
    MSelectionList selectionList;
    status = selectionList.add(refNodeName);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
    status = selectionList.getDependNode(0, mObj);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
    return mObj;
}

MObject
PxrUsdMayaUtil::GetReferenceNode(const MObject& mobj)
{
    MStatus status;
    if (mobj.hasFn(MFn::kDagNode)) {
        MFnDagNode mfn(mobj, &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
        if (!mfn.isFromReferencedFile()) return MObject::kNullObj;
        return GetReferenceNode(mfn.partialPathName());
    }
    else {
        MFnDependencyNode mfn(mobj, &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
        if (!mfn.isFromReferencedFile()) return MObject::kNullObj;
        return GetReferenceNode(mfn.name());
    }
}

bool
PxrUsdMayaUtil::IsUsdReference(const MObject& mobj)
{
    // Would like to know a way to directly query a reference for what file type
    // or translator was used to create it; however, since no such method seems
    // to exist, we fall back on using usdTranslator::identifyFile. This is not
    // foolproof, as in theory, there could be multiple translators registered
    // for .usd files...
    MStatus status;
    MFnReference mfnRef(mobj, &status);
    if (!status) return false;
    MString fileName = mfnRef.fileName(
            true, // resolvedName
            true,  // includePath
            false, // includeCopyNumber,
            &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    return UsdStage::IsSupportedFile(fileName.asChar());
}

bool
PxrUsdMayaUtil::IsNativeMayaReference(const MObject& mobj)
{
    MStatus status;
    MFnReference mfnRef(mobj, &status);
    if (!status) return false;
    MString fileName = mfnRef.fileName(
            true, // resolvedName
            true,  // includePath
            false, // includeCopyNumber,
            &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    std::string extension = TfStringToLower(TfStringGetSuffix(fileName.asChar()));
    if (extension != "ma" && extension != "mb")
    {
        return false;
    }
}

std::vector<MObject>
PxrUsdMayaUtil::FullReferenceChain(const MObject& origRefObj)
{
    MStatus status;
    MObject currentRefObj = origRefObj;
    std::vector<MObject> refs;
    MFnReference mfnRef;

    while (!currentRefObj.isNull()) {
        status = mfnRef.setObject(currentRefObj);
        if (!status) break;
        refs.push_back(currentRefObj);
        currentRefObj = mfnRef.parentReference(&status);
        if (!status) break;
    }
    return refs;
}

// TODO: either get some sort of proof this works, or go to a system where
// we store MObject => original USD path mapping. Additional considerations
// are the fact that top-level-reference nodes may be reparented to a
// non-referenced node (though THAT node may subsequently saved to a scene
// which IS referenced, so...)

// TODO: figure out what to do with a USD reference to an abc file - these may
// "look" like a abc reference (because of it's extension) in maya... because
// I don't know of a way to query the ACTUAL file translator used to create
// a reference!

// TODO: make work with assemblies?
std::string
PxrUsdMayaUtil::GetUsdNamespace(const MObject& mobj)
{
    // Ok, the conceptual goal here is that, if a node was originally a usd
    // node, but had namespaces added by the usd-maya plugin, that on export,
    // we strip out the namespaces we added, so we end up with the same original
    // usd path on export. (Ie, round-tripping.)

    // If the node is the direct result of a usd assembly / reference, (and
    // all of it's parent assemblies / references are usd assemblies /
    // references) then the logic becomes simple: usd has no namespaces (ie,
    // you can't have a ':' in the name), so any namespaces MUST have been
    // added by the usd-maya plugin, and we can strip ALL namespaces to end up
    // with the original path.

    // However, the introduction of native-maya-references complicates things
    // slightly.  As soon as we have a native maya file, it might introduce
    // arbitrary namespaces of it's own... so the only safe thing to do is
    // to strip out the namespace assigned to the native maya reference itself,
    // but nothing else.

    // An additional wrinkle is the mixing of multiple levels of referencing,
    // both usd and native-maya. Ie, say we have this referencing chain
    // (from top to bottom):
    //
    //    usd1 (ns: None) => maya2 (ns: top:mayaObj) => maya3 (ns: subMaya)
    //
    // The "maya3" reference (whose total namespace is "top:mayaObj:subMaya")
    // was created by a normal maya reference in "maya2" - and so it's "subMaya"
    // namespace has (potentially) no relationship to USD paths, and so must
    // be preserved to be safe.  The maya2 reference, however, is a "native maya"
    // reference created by usd - and so it's namespace (top:mayaObj) is "safe"
    // to remove from all it's objects (and all the objects in it's
    // subreferences, ie, maya3).

    // Now consider this chain:
    //    usd1 (ns: None) => maya2 (ns: top:mayaObj) => maya3 (ns: subMaya) => usd4 (ns: None)
    // For objects in usd3, it is NOT safe to strip all namespaces, as the
    // "subMaya" part does not come from usd. However, it should be safe to strip
    // the namespace from maya2.

    // I think the common logic here is that we go to the top of the reference
    // chain, and keep going downstream, looking for non-usd refs. If ALL the
    // refs are USD, we can strip all namespaces from this node. If we find at
    // least one USD ref, stop at the first non-usd ref - if THAT is a maya ref,
    // then it is usd-native-maya ref, and we assume we can strip that ref's
    // namespace.

    // Finally, if it's a dag node, these rules apply ONLY for the node name
    // of this exact node - to get the full dag path, we have to repeat this
    // process for all parent nodes.

    MStatus status;

    MFnDependencyNode mfnDep(mobj, &status);
    if (!status) return "";

    std::string name = TfStringTrim(mfnDep.name().asChar(), ":");

    // If there's no namespace, no sense worrying about anything else!
    size_t last_colon = name.rfind(':');
    if (last_colon == std::string::npos) return "";

    MObject refObj = GetReferenceNode(mobj);
    if (refObj.isNull()) return "";

    std::vector<MObject> refs = FullReferenceChain(refObj);
    bool foundUsd = false;
    MObject mayaRefMObj;

    // refs are in reverse order, from innermost to outermost (bottom to top)
    for (auto reverseIt = refs.rbegin(); reverseIt != refs.rend(); ++reverseIt)
    {
        MObject& refMObj = *reverseIt;
        if(IsUsdReference(refMObj)) {
            foundUsd = true;
        }
        else if(IsNativeMayaReference(refMObj)) {
            if (foundUsd) {
                mayaRefMObj = refMObj;
            }
            break;
        }
        else {
            // If we found something other than a maya or USD ref, assume we alter
            // touch anything
            return "";
        }
    }

    if (!mayaRefMObj.isNull()) {
        // We found a maya ref, that has ONLY usd refs above it - strip it's
        // namespace, and nothing else...
        MFnReference mfnRef(mayaRefMObj, &status);
        CHECK_MSTATUS_AND_RETURN(status, "");
        std::string toStrip = TfStringTrimLeft(mfnRef.associatedNamespace(false, &status).asChar(),
                                               ":");
        if (toStrip.back() != ':') toStrip += ':';
        if (!status) return "";
        if (TfStringStartsWith(name, toStrip)) {
            return toStrip;
        }
    }

    if (foundUsd) {
        // If foundUsd is True, but usdMayaRef wasn't set, then ALL refs were
        // usd refs, and we assume that usd added ALL namespaces
        return name.substr(0, last_colon+1);
    }
    return "";
}

std::string
PxrUsdMayaUtil::MDagPathToString(
        const MDagPath& dagPath,
        bool stripUsdNamespaces)
{
    std::string usdPathStr;
    if (stripUsdNamespaces) {
        // For each node in the dag path, strip usd-added namespaces...
        MDagPath remainingPath = dagPath;
        std::vector<std::string> pathElements;
        while (remainingPath.length())
        {
            MObject mobj = remainingPath.node();
            remainingPath.pop();
            // Due to underworld nodes, it's possible to get an "underworld root"
            // "node" which has no name...
            std::string nodeName = MFnDependencyNode(mobj).name().asChar();
            if (!nodeName.length()) continue;
            std::string toStrip = GetUsdNamespace(mobj);
            if (toStrip.length()) {
                nodeName = nodeName.substr(toStrip.length());
            }
            pathElements.push_back(nodeName);
        }
        // Add a final empty string, so after joining, we have a leading "/"
        pathElements.push_back("");

        // The path elements are in reverse order, so join with the reversed
        // iterator...
        usdPathStr = TfStringJoin(pathElements.rbegin(), pathElements.rend(), "/");
    }
    else {
        usdPathStr = dagPath.fullPathName().asChar();
    }

    std::replace( usdPathStr.begin(), usdPathStr.end(), '|', '/');
    // We may want to have another option that allows us to drop namespace's
    // instead of making them part of the path.
    std::replace( usdPathStr.begin(), usdPathStr.end(), ':', '_'); // replace namespace ":" with "_"
    // Replacing characters because of underworld
    usdPathStr = std::string(usdPathStr.begin(), std::remove( usdPathStr.begin(), usdPathStr.end(), '-'));
    usdPathStr = std::string(usdPathStr.begin(), std::remove( usdPathStr.begin(), usdPathStr.end(), '>'));
    return usdPathStr;
}

SdfPath
PxrUsdMayaUtil::MDagPathToUsdPath(
        const MDagPath& dagPath,
        bool mergeTransformAndShape,
        bool stripUsdNamespaces)
{
    SdfPath usdPath(MDagPathToString(dagPath, stripUsdNamespaces));
    if (mergeTransformAndShape && _IsShape(dagPath)) {
        usdPath = usdPath.GetParentPath();
    }

    return usdPath;
}

bool PxrUsdMayaUtil::GetBoolCustomData(UsdAttribute obj, TfToken key, bool defaultValue)
{
    bool returnValue=defaultValue;
    VtValue data = obj.GetCustomDataByKey(key);
    if (!data.IsEmpty()) {
        if (data.IsHolding<bool>()) {
            return data.Get<bool>();
        } else {
            MGlobal::displayError("Custom Data: " + MString(key.GetText()) +
                                    " is not of type bool. Skipping...");
        }
    }
    return returnValue;
}

template <typename T>
static T
_GetVec(
        const UsdAttribute& attr,
        const VtValue& val)
{
    T ret = val.UncheckedGet<T>();
    if (attr.GetRoleName() == SdfValueRoleNames->Color)  {
        return GfConvertLinearToDisplay(ret);
    }   
    return ret;

}

bool
PxrUsdMayaUtil::setPlugValue(
        const UsdAttribute& usdAttr,
        MPlug& attrPlug)
{
    return setPlugValue(usdAttr, UsdTimeCode::Default(), attrPlug);
}

bool
PxrUsdMayaUtil::setPlugValue(
        const UsdAttribute& usdAttr,
        UsdTimeCode time,
        MPlug& attrPlug)
{
    MStatus status = MStatus::kFailure;
    VtValue val;
    if (usdAttr.Get(&val, time)) {
        if (val.IsHolding<float>()) {
            status = attrPlug.setFloat(val.UncheckedGet<float>());
        }
        else if (val.IsHolding<GfVec3f>()) {
            if (attrPlug.isCompound()) {
                GfVec3f vec3fVal = _GetVec<GfVec3f>(usdAttr, val);
                for (int i = 0; i < 3; i++) {
                    MPlug childPlug = attrPlug.child(i, &status);
                    CHECK_MSTATUS_AND_RETURN(status, false);
                    status = childPlug.setFloat(vec3fVal[i]);
                    CHECK_MSTATUS_AND_RETURN(status, false);
                }
            }
        }
        else if (val.IsHolding<bool>()) {
            status = attrPlug.setBool(
                val.UncheckedGet<bool>());
        }
        else if (val.IsHolding<std::string>()) {
            status = attrPlug.setString(
                MString(val.UncheckedGet<std::string>().c_str()));
        }
        else if (val.IsHolding<TfToken>()) {
            TfToken token(val.UncheckedGet<TfToken>());
            MObject attrObj = attrPlug.attribute(&status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            if (attrObj.hasFn(MFn::kEnumAttribute)) {
                MFnEnumAttribute attrEnumFn(attrObj, &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                short enumVal = attrEnumFn.fieldIndex(MString(token.GetText()), &status);
                CHECK_MSTATUS_AND_RETURN(status, false);
                status = attrPlug.setShort(enumVal);
            }
        }
    }

    CHECK_MSTATUS_AND_RETURN(status, false);

    return true;
}

bool
PxrUsdMayaUtil::createStringAttribute(
        MFnDependencyNode& depNode,
        const MString& attr) {
    MStatus status = MStatus::kFailure;
    MFnTypedAttribute typedAttrFn;
    MObject attrObj = typedAttrFn.create(
            attr,
            attr,
            MFnData::kString,
            MObject::kNullObj,
            &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    
    status = depNode.addAttribute(attrObj);
    CHECK_MSTATUS_AND_RETURN(status, false);
    
    return true;
}

bool
PxrUsdMayaUtil::createNumericAttribute(
        MFnDependencyNode& depNode,
        const MString& attr,
        MFnNumericData::Type type) {
    MStatus status = MStatus::kFailure;
    MFnNumericAttribute numericAttrFn;
    MObject attrObj = numericAttrFn.create(
            attr,
            attr,
            type,
            0,
            &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    
    status = depNode.addAttribute(attrObj);
    CHECK_MSTATUS_AND_RETURN(status, false);
    
    return true;
}
