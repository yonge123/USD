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
{
}

RefEditUtil::RefEditUtil(const bool mergeTransformAndShape):
        mMergeTransformAndShape(mergeTransformAndShape)
{
}

RefEditUtil::~RefEditUtil()
{
}

bool
RefEditUtil::GetDagNodeEdits(const MDagPath& dagPath, RefEdits& outEdits)
{
    // Get the Reference / Assembly that this node belongs to
    // TODO: Add ref/assembly switch
    MObject referenceObj = PxrUsdMayaUtil::GetReferenceNode(dagPath.node());

    // TODO: We probably only want to consider a node "referenced" if it is
    // from a usd file reference, because the overs will need to be layered
    // over a usd file.
    if (!referenceObj.isNull()) {
        outEdits.isReferenced = true;
        MFnDependencyNode refNode(referenceObj);
        // Check if we have not yet processed this ref and its edits
        if (mReferences.find(refNode.name().asChar()) == mReferences.end()) {
            // Get Edits from a reference / Assembly
            ProcessReference(referenceObj);
            mReferences.insert(refNode.name().asChar());
        }

        SdfPath usdPath;
        usdPath = PxrUsdMayaUtil::MDagPathToUsdPath(dagPath, mMergeTransformAndShape);
        if (!TfMapLookup(mPrimPathToRefEdits, usdPath, &outEdits.modifiedAttrs)) {
            return false;
        }
    }
    return true;
}

void
RefEditUtil::ProcessReference(
        const MObject& referenceObj)
{
    MObject editsOwner(referenceObj);
    MObject targetNode(referenceObj);
    MItEdits assemEdits(editsOwner, targetNode);

    while (!assemEdits.isDone()) {
        MEdit::EditType editType = assemEdits.currentEditType();
        if (editType == MEdit::kSetAttrEdit) {
            MSetAttrEdit mSetAttrEdit = assemEdits.setAttrEdit();

            MStatus status;
            MPlug mPlug = mSetAttrEdit.plug(&status);
            if (status) {
                MObject editedNode = mPlug.node();
                std::string attrName = mPlug.name().asChar();

                // Get the attribute name of "node.attribute"
                std::vector<std::string> attrSplit = TfStringSplit(attrName, ".");
                if (attrSplit.size() == 2) {
                    attrName = attrSplit[1];
                }

                MDagPath editPath;
                MDagPath::getAPathTo(editedNode, editPath);
                SdfPath usdPath;
                usdPath = PxrUsdMayaUtil::MDagPathToUsdPath(editPath,
                                                            mMergeTransformAndShape);
                mPrimPathToRefEdits[usdPath].insert(attrName);
            }
        }
        assemEdits.next();
    }
}