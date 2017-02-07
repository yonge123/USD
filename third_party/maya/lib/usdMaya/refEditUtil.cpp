//
// Created by nickk on 1/31/17.
//

#include "refEditUtil.h"
#include "usdMaya/util.h"
#include <maya/MFnAssembly.h>
#include <maya/MFnReference.h>

#include <maya/MItEdits.h>
#include <maya/MEdit.h>
#include <maya/MSetAttrEdit.h>

RefEdits::RefEdits(): isReferenced(false)
{}

RefEditUtil::RefEditUtil(bool mergeTransformAndShape, bool stripUsdNamespaces):
        _mergeTransformAndShape(mergeTransformAndShape),
        _stripUsdNamespaces(stripUsdNamespaces)
{
}

RefEditUtil::~RefEditUtil(){
}

void
RefEditUtil::GetDagNodeEdits(const MDagPath& dagPath, RefEdits& outEdits){

    // Get the Reference / Assembly that this node belongs to
    // TODO: Add ref/assembly switch
    MObject referenceObj = PxrUsdMayaUtil::GetReferenceNode(dagPath.node());

    if (!referenceObj.isNull()) {
        outEdits.isReferenced = true;

        MStatus status;
        MFnDependencyNode refNode(referenceObj, &status);

        // Check if we have not yet processed this ref and its edits
        if (_references.find(refNode.name().asChar()) == _references.end()) {
            // Get Edits from a reference / Assembly
            ProcessReference(referenceObj);
            _references.insert(refNode.name().asChar());
        }

        SdfPath usdPath;
        usdPath = PxrUsdMayaUtil::MDagPathToUsdPath(dagPath,
                                                    _mergeTransformAndShape,
                                                    _stripUsdNamespaces);

        if (!TfMapLookup(_primPathToRefEdits, usdPath, &outEdits.modifiedAttrs)) {
            std::cout << "found no edits for path";
        }
    }
}


void
RefEditUtil::ProcessReference(
        const MObject &mObj)
{
    MStatus status;

    MObject editsOwner(mObj);
    MObject targetNode(mObj);

    MItEdits assemEdits(editsOwner, targetNode);

    while( !assemEdits.isDone() ) {
        MStatus status;
        MEdit::EditType editType;
        editType = assemEdits.currentEditType(&status);

        if (editType == MEdit::kSetAttrEdit){
            MSetAttrEdit mSetAttrEdit = assemEdits.setAttrEdit(&status);
            MPlug mPlug = mSetAttrEdit.plug(&status);

            if (status) {
                MObject editedNode = mPlug.node(&status);
                std::string attrName = mPlug.name(&status).asChar();

                // Only keep the attribute name of node.attribute
                attrName = TfStringSplit(attrName, ".")[1];

                MDagPath editPath;
                MDagPath::getAPathTo(editedNode, editPath);

                SdfPath usdPath;
                usdPath = PxrUsdMayaUtil::MDagPathToUsdPath(editPath,
                                                            _mergeTransformAndShape,
                                                            _stripUsdNamespaces);

                _primPathToRefEdits[usdPath].insert(attrName);
            }
        }

        assemEdits.next();
    }
}