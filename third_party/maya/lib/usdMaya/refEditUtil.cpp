//
// Created by nickk on 1/31/17.
//

#include "refEditUtil.h"
#include "util.h"
#include <maya/MFnAssembly.h>
#include <maya/MFnReference.h>

#include <maya/MItEdits.h>
#include <maya/MEdit.h>
#include <maya/MSetAttrEdit.h>

RefEditUtil::RefEditUtil(){
}

void
RefEditUtil::GetDagNodeEdits(const MDagPath& dagPath, RefEdits& outEdits){

    // Get the Reference / Assembly that this node belongs to
    // Add switch
    MObject referenceObj = PxrUsdMayaUtil::GetReferenceNode(dagPath);

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

        // debug
        MDagPath refDagPath;
        MDagPath::getAPathTo(referenceObj, refDagPath);
        std::cout << "dag path: " << dagPath.fullPathName() << endl;
        std::cout << "ref obj dag path: " << refDagPath.fullPathName() << endl;

        if (!TfMapLookup(_dagPathToRefEdits, dagPath, &outEdits.modifiedAttrs)) {
            std::cout << "found no edits for path";
        }

    }
}


//            // Could try to reuse Proxy shape ref edit parsing...
//            PxrUsdMayaEditUtil::PathEditMap *refEdits;
//            std::vector<std::string> invalidEdits, failedEdits;
//
//            // TODO: would need to make this work with referencess
//            PxrUsdMayaEditUtil::GetEditsForAssembly(assemObj, &refEdits, &invalidEdits);
void
RefEditUtil::ProcessReference(
        const MObject &mObj)
{
    MStatus status;

    // REF / Assembly switch
//    if (mObj.hasFn(MFn::kAssembly)){
//        MFnAssembly assemblyFn(mObj, &status);
//        if( !status )
//            return;
//
//        MObject editsOwner(mObj);
//        MObject targetNode(mObj);
//
//    } else {
//        MFnReference referenceFn(mObj, &status);
//        if( !status )
//            return;
//        // TODO: find top level ref
//        MObject editsOwner(mObj);
//        // argument ref
//        MObject targetNode(mObj);
//    }
    MObject editsOwner(mObj);
    MObject targetNode(mObj);

    MItEdits assemEdits(editsOwner, targetNode);

    while( !assemEdits.isDone() ) {
        MStatus status;
        MSetAttrEdit mSetAttrEdit = assemEdits.setAttrEdit(&status);
        if (!status){
            // More connection types here
            continue;
        }

        // TODO: add checking
        MPlug mPlug = mSetAttrEdit.plug(&status);
        if (!status){
            continue;
        }
        MObject mObj = mPlug.node(&status);
        std::string attrName = mPlug.name(&status).asChar();

        // Only keep the attribute name of node.attribute
        attrName = TfStringSplit(attrName, ".")[-1];

        MDagPath editPath;
        MDagPath::getAPathTo(mObj, editPath);

//        (*_dagPathToRefEdits)[editPath].push_back(attrName);
        _dagPathToRefEdits[editPath].insert(attrName);

        assemEdits.next();
    }
}