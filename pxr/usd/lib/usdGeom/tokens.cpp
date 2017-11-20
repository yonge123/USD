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
#include "pxr/usd/usdGeom/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdGeomTokensType::UsdGeomTokensType() :
    all("all"),
    angularVelocities("angularVelocities"),
    axis("axis"),
    basis("basis"),
    best("best"),
    bezier("bezier"),
    bilinear("bilinear"),
    boundaries("boundaries"),
    bounds("bounds"),
    box("box"),
    bspline("bspline"),
    camera("camera"),
    cards("cards"),
    catmullClark("catmullClark"),
    catmullRom("catmullRom"),
    clippingPlanes("clippingPlanes"),
    clippingRange("clippingRange"),
    closed("closed"),
    collection("collection"),
    constant("constant"),
    cornerIndices("cornerIndices"),
    cornerSharpnesses("cornerSharpnesses"),
    cornersOnly("cornersOnly"),
    cornersPlus1("cornersPlus1"),
    cornersPlus2("cornersPlus2"),
    coverage("coverage"),
    coverageOrigin("coverageOrigin"),
    creaseIndices("creaseIndices"),
    creaseLengths("creaseLengths"),
    creaseSharpnesses("creaseSharpnesses"),
    cross("cross"),
    cubic("cubic"),
    curveVertexCounts("curveVertexCounts"),
    default_("default"),
    doubleSided("doubleSided"),
    edgeAndCorner("edgeAndCorner"),
    edgeOnly("edgeOnly"),
    elementSize("elementSize"),
    elementType("elementType"),
    extent("extent"),
    extentsHint("extentsHint"),
    face("face"),
    faceSet("faceSet"),
    faceVarying("faceVarying"),
    faceVaryingLinearInterpolation("faceVaryingLinearInterpolation"),
    faceVertexCounts("faceVertexCounts"),
    faceVertexIndices("faceVertexIndices"),
    familyName("familyName"),
    fill("fill"),
    fit("fit"),
    focalLength("focalLength"),
    focusDistance("focusDistance"),
    frame("frame"),
    fromTexture("fromTexture"),
    fStop("fStop"),
    guide("guide"),
    height("height"),
    hermite("hermite"),
    holeIndices("holeIndices"),
    horizontal("horizontal"),
    horizontalAperture("horizontalAperture"),
    horizontalApertureOffset("horizontalApertureOffset"),
    ids("ids"),
    inactiveIds("inactiveIds"),
    indices("indices"),
    infoFilename("info:filename"),
    inherited("inherited"),
    interpolateBoundary("interpolateBoundary"),
    interpolation("interpolation"),
    invisible("invisible"),
    invisibleIds("invisibleIds"),
    knots("knots"),
    left("left"),
    leftHanded("leftHanded"),
    linear("linear"),
    loop("loop"),
    modelApplyDrawMode("model:applyDrawMode"),
    modelCardGeometry("model:cardGeometry"),
    modelCardTextureXNeg("model:cardTextureXNeg"),
    modelCardTextureXPos("model:cardTextureXPos"),
    modelCardTextureYNeg("model:cardTextureYNeg"),
    modelCardTextureYPos("model:cardTextureYPos"),
    modelCardTextureZNeg("model:cardTextureZNeg"),
    modelCardTextureZPos("model:cardTextureZPos"),
    modelDrawMode("model:drawMode"),
    modelDrawModeColor("model:drawModeColor"),
    mono("mono"),
    motionVelocityScale("motion:velocityScale"),
    none("none"),
    nonOverlapping("nonOverlapping"),
    nonperiodic("nonperiodic"),
    normals("normals"),
    offset("offset"),
    open("open"),
    order("order"),
    orientation("orientation"),
    orientations("orientations"),
    origin("origin"),
    orthographic("orthographic"),
    partition("partition"),
    periodic("periodic"),
    perspective("perspective"),
    points("points"),
    pointWeights("pointWeights"),
    positions("positions"),
    power("power"),
    primvarsDisplayColor("primvars:displayColor"),
    primvarsDisplayOpacity("primvars:displayOpacity"),
    projection("projection"),
    protoIndices("protoIndices"),
    prototypes("prototypes"),
    proxy("proxy"),
    proxyPrim("proxyPrim"),
    purpose("purpose"),
    radius("radius"),
    ranges("ranges"),
    render("render"),
    right("right"),
    rightHanded("rightHanded"),
    rotate("rotate"),
    scales("scales"),
    shutterClose("shutter:close"),
    shutterOpen("shutter:open"),
    size("size"),
    smooth("smooth"),
    stereoRole("stereoRole"),
    subdivisionScheme("subdivisionScheme"),
    toSize("to size"),
    triangleSubdivisionRule("triangleSubdivisionRule"),
    trimCurveCounts("trimCurve:counts"),
    trimCurveKnots("trimCurve:knots"),
    trimCurveOrders("trimCurve:orders"),
    trimCurvePoints("trimCurve:points"),
    trimCurveRanges("trimCurve:ranges"),
    trimCurveVertexCounts("trimCurve:vertexCounts"),
    type("type"),
    uForm("uForm"),
    uKnots("uKnots"),
    unauthoredValuesIndex("unauthoredValuesIndex"),
    uniform("uniform"),
    unrestricted("unrestricted"),
    uOrder("uOrder"),
    upAxis("upAxis"),
    uRange("uRange"),
    uVertexCount("uVertexCount"),
    varying("varying"),
    velocities("velocities"),
    vertex("vertex"),
    vertical("vertical"),
    verticalAperture("verticalAperture"),
    verticalApertureOffset("verticalApertureOffset"),
    vForm("vForm"),
    visibility("visibility"),
    vKnots("vKnots"),
    vOrder("vOrder"),
    vRange("vRange"),
    vVertexCount("vVertexCount"),
    widths("widths"),
    wrap("wrap"),
    x("X"),
    xformOpOrder("xformOpOrder"),
    y("Y"),
    z("Z"),
    allTokens({
        all,
        angularVelocities,
        axis,
        basis,
        best,
        bezier,
        bilinear,
        boundaries,
        bounds,
        box,
        bspline,
        camera,
        cards,
        catmullClark,
        catmullRom,
        clippingPlanes,
        clippingRange,
        closed,
        collection,
        constant,
        cornerIndices,
        cornerSharpnesses,
        cornersOnly,
        cornersPlus1,
        cornersPlus2,
        coverage,
        coverageOrigin,
        creaseIndices,
        creaseLengths,
        creaseSharpnesses,
        cross,
        cubic,
        curveVertexCounts,
        default_,
        doubleSided,
        edgeAndCorner,
        edgeOnly,
        elementSize,
        elementType,
        extent,
        extentsHint,
        face,
        faceSet,
        faceVarying,
        faceVaryingLinearInterpolation,
        faceVertexCounts,
        faceVertexIndices,
        familyName,
        fill,
        fit,
        focalLength,
        focusDistance,
        frame,
        fromTexture,
        fStop,
        guide,
        height,
        hermite,
        holeIndices,
        horizontal,
        horizontalAperture,
        horizontalApertureOffset,
        ids,
        inactiveIds,
        indices,
        infoFilename,
        inherited,
        interpolateBoundary,
        interpolation,
        invisible,
        invisibleIds,
        knots,
        left,
        leftHanded,
        linear,
        loop,
        modelApplyDrawMode,
        modelCardGeometry,
        modelCardTextureXNeg,
        modelCardTextureXPos,
        modelCardTextureYNeg,
        modelCardTextureYPos,
        modelCardTextureZNeg,
        modelCardTextureZPos,
        modelDrawMode,
        modelDrawModeColor,
        mono,
        motionVelocityScale,
        none,
        nonOverlapping,
        nonperiodic,
        normals,
        offset,
        open,
        order,
        orientation,
        orientations,
        origin,
        orthographic,
        partition,
        periodic,
        perspective,
        points,
        pointWeights,
        positions,
        power,
        primvarsDisplayColor,
        primvarsDisplayOpacity,
        projection,
        protoIndices,
        prototypes,
        proxy,
        proxyPrim,
        purpose,
        radius,
        ranges,
        render,
        right,
        rightHanded,
        rotate,
        scales,
        shutterClose,
        shutterOpen,
        size,
        smooth,
        stereoRole,
        subdivisionScheme,
        toSize,
        triangleSubdivisionRule,
        trimCurveCounts,
        trimCurveKnots,
        trimCurveOrders,
        trimCurvePoints,
        trimCurveRanges,
        trimCurveVertexCounts,
        type,
        uForm,
        uKnots,
        unauthoredValuesIndex,
        uniform,
        unrestricted,
        uOrder,
        upAxis,
        uRange,
        uVertexCount,
        varying,
        velocities,
        vertex,
        vertical,
        verticalAperture,
        verticalApertureOffset,
        vForm,
        visibility,
        vKnots,
        vOrder,
        vRange,
        vVertexCount,
        widths,
        wrap,
        x,
        xformOpOrder,
        y,
        z
    })
{
}

TfStaticData<UsdGeomTokensType> UsdGeomTokens;

PXR_NAMESPACE_CLOSE_SCOPE
