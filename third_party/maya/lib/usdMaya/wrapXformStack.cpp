//
// Copyright 2017 Pixar
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
#include "pxr/pxr.h"
#include "usdMaya/xformStack.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyEnum.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MEulerRotation.h>
#include <maya/MTransformationMatrix.h>

#include <boost/python/class.hpp>
#include <boost/python/import.hpp>
#include <boost/python/raw_function.hpp>

using namespace std;
using boost::python::object;
using boost::python::class_;
using boost::python::no_init;
using boost::python::self;
using boost::python::extract;
using boost::python::return_value_policy;
using boost::python::return_by_value;
using boost::python::reference_existing_object;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {
    constexpr const char RETURN_ROT_ORDER[] = "returnRotOrder";

    constexpr const char RETURN_ROT_ORDER_DOCSTRING_TEMPLATE[] = \
            "You may pass '%s=True' as a kwarg to also return the inferred rotation\n"
            "order as a second return result.  The rotation order is returned as an\n"
            "integer sutiable for use with transformNode.rotateOrder attribute - ie,\n"
            "XYZ=0, YZX=1, ZXY=2, XZY=3, YXZ=4, and ZYX=5. It will be set if there is a\n"
            "three-axis euler rotation present in the input UsdGeomXformOps; if None is\n"
            "present, XYZ (0) will be returned by defaut";

    const std::string RETURN_ROT_ORDER_DOCSTRING = TfStringPrintf(
            RETURN_ROT_ORDER_DOCSTRING_TEMPLATE, RETURN_ROT_ORDER);

    const std::string MATCHING_SUBSTACK_DOCSTRING = TfStringPrintf(
            "Returns a list of matching XformOpDefinitions for this stack\n"
            "\n"
            "For each xformop, we want to find the corresponding op within this\n"
            "stack that it matches.  There are 3 requirements:\n"
            " - to be considered a match, the name and type must match an op in this stack\n"
            " - the matches for each xformop must have increasing indexes in the stack\n"
            " - inversionTwins must either both be matched or neither matched.\n"
            "\n"
            "This returns a vector of pointers to the matching XformOpDefinitions in this\n"
            "stack. The size of this vector will be 0 if no complete match is found,\n"
            "or xformops.size() if a complete match is found.\n"
            "\n"
            "%s",
            RETURN_ROT_ORDER_DOCSTRING.c_str());

    const std::string FIRST_MATCHING_DOCSTRING = TfStringPrintf(
            "Runs MatchingSubstack against the given list of stacks\n"
            "\n"
            "Returns the first non-empty result it finds; if all stacks\n"
            "return an empty vector, an empty vector is returned.\n"
            "The first arg must be the list of UsdGeomXformOps to match;\n"
            "the remaining args are the PxrUsdMayaXformStack objects to test against,\n"
            "in order.\n"
            "\n"
            "%s",
            RETURN_ROT_ORDER_DOCSTRING.c_str());


    class Usd_PyXformOpClassification {
    public:
        // to-python conversion of const PxrUsdMayaXformOpClassification.
        static PyObject*
        convert(const PxrUsdMayaXformOpClassification& opClass) {
            TfPyLock lock;
            // (extra parens to avoid 'most vexing parse')
            object obj((PxrUsdMayaXformOpClassificationConstPtr(&opClass)));
            PyObject* result = obj.ptr();
            // Incref because ~object does a decref
            boost::python::incref(result);
            return result;
        }

        // to-python conversion of rvalue-ref of PxrUsdMayaXformOpClassification.
        static PyObject*
        convert(PxrUsdMayaXformOpClassification&& opClass) {
            // This is just here as a guard... We shouldn't be creating new
            // PxrUsdMayaXformOpClassification objects in python, so raise an error!
            throw boost::enable_current_exception(std::runtime_error(
                    "Cannot convert a PxrUsdMayaXformOpClassification object to python by value"));
            // Just here to stifle compiler warnings...
            return nullptr;
        }

        // In order to return wrapped UsdGeomXformOp::Type objects, we need to import
        // the UsdGeom module
        static void
        ImportUsdGeomOnce()
        {
            static std::once_flag once;
            std::call_once(once, [](){
                boost::python::import("pxr.UsdGeom");
            });
        }

        static UsdGeomXformOp::Type
        GetOpType(const PxrUsdMayaXformOpClassification& opClass)
        {
            ImportUsdGeomOnce();
            return opClass.GetOpType();
        }
    };

    class Usd_PyXformStack
    {
    public:
        static inline object
        convert_index(size_t index)
        {
            if (index == PxrUsdMayaXformStack::NO_INDEX)
            {
                return object(); // return None (handles the incref)
            }
            return object(index);
        }

        // Don't want to make this into a generic conversion rule, via
        //    to_python_converter<PxrUsdMayaXformStack::IndexPair, Usd_PyXformStack>(),
        // because that would make this apply to ANY pair of unsigned ints, which
        // could be dangerous
        static object
        convert_index_pair(const PxrUsdMayaXformStack::IndexPair& indexPair)
        {
            return boost::python::make_tuple(
                    convert_index(indexPair.first),
                    convert_index(indexPair.second));
        }

        static PyObject*
        convert(const PxrUsdMayaXformStack::OpClassPtrPair& opPair)
        {
            boost::python::tuple result = boost::python::make_tuple(
                    opPair.first, opPair.second);
            return boost::python::incref(result.ptr());
        }

        static const PxrUsdMayaXformOpClassification&
        getitem(const PxrUsdMayaXformStack& stack, long index)
        {
            auto raise_index_error = [] () {
                PyErr_SetString(PyExc_IndexError, "index out of range");
                boost::python::throw_error_already_set();
            };

            if (index < 0)
            {
                if (static_cast<size_t>(-index) > stack.GetSize()) raise_index_error();
                return stack[stack.GetSize() + index];
            }
            else
            {
                if (static_cast<size_t>(index) >= stack.GetSize()) raise_index_error();
                return stack[index];
            }
        }

        static object
        GetInversionTwins(const PxrUsdMayaXformStack& stack)
        {
            boost::python::list result;
            for(const auto& idxPair : stack.GetInversionTwins())
            {
                result.append(convert_index_pair(idxPair));
            }
            return result;
        }

        static object
        FindOpIndex(
                const PxrUsdMayaXformStack& stack,
                const TfToken& opName,
                bool isInvertedTwin=false)
        {
            return convert_index(stack.FindOpIndex(opName, isInvertedTwin));
        }

        static object
        FindOpIndexPair(
                const PxrUsdMayaXformStack& stack,
                const TfToken& opName)
        {
            return convert_index_pair(stack.FindOpIndexPair(opName));
        }

        static object
        MatchingSubstack(
                const PxrUsdMayaXformStack& stack,
                const std::vector<UsdGeomXformOp>& xformops,
                bool returnRotOrder)
        {
            MTransformationMatrix::RotationOrder rotOrder = MTransformationMatrix::kXYZ;
            std::vector<PxrUsdMayaXformStack::OpClassPtr> result = \
                    stack.MatchingSubstack(xformops, &rotOrder);

            boost::python::list matching_list;
            if (!result.empty())
            {
                for(auto& op : result)
                {
                    matching_list.append(op);
                }
            }
            if (returnRotOrder)
            {
                // set outRotate to value usable with transformNode.rotateOrder attribute,
                // if invalid, just return kXYZ
                int outRotate;
                switch(rotOrder) {
                case MTransformationMatrix::kXYZ:
                    outRotate = MEulerRotation::kXYZ;
                    break;
                case MTransformationMatrix::kYZX:
                    outRotate = MEulerRotation::kYZX;
                    break;
                case MTransformationMatrix::kZXY:
                    outRotate = MEulerRotation::kZXY;
                    break;
                case MTransformationMatrix::kXZY:
                    outRotate = MEulerRotation::kXZY;
                    break;
                case MTransformationMatrix::kYXZ:
                    outRotate = MEulerRotation::kYXZ;
                    break;
                case MTransformationMatrix::kZYX:
                    outRotate = MEulerRotation::kZYX;
                    break;
                default:
                    outRotate = MEulerRotation::kXYZ;
                    break;
                }
                return boost::python::make_tuple(matching_list, outRotate);
            }
            else
            {
                return matching_list;
            }
        }

        static object
        FirstMatchingSubstack(boost::python::tuple args, boost::python::dict kwargs)
        {
            // WARNING: this logic is currently duplicated in xformStack.h,
            // PxrUsdMayaXformStack::FirstMatchingSubstack, because it was much
            // simpler / readable than using variadic templates to convert a
            // python tuple into variadic arguments. Should be ok because the
            // logic is currently so simple. If this logic changes, either change it
            // in both places, or modify the python wrapper to call the variadic
            // template function directly.
            bool returnRotOrder = false;
            if (len(kwargs) > 0)
            {
                object returnRotOrderValue = kwargs.get("returnRotOrder");
                if (returnRotOrderValue.is_none() || len(kwargs) > 1)
                {
                    std::string msg = std::string("only allowable kwarg to FirstMatchingSubstack is ") \
                            + RETURN_ROT_ORDER;
                    PyErr_SetString(PyExc_ValueError, msg.c_str());
                    boost::python::throw_error_already_set();
                }
                returnRotOrder = extract<bool>(returnRotOrderValue);
            }

            if (len(args) < 1)
            {
                PyErr_SetString(PyExc_ValueError,
                        "FirstMatchingSubstack requires a list of of XformOps as the first arg");
                boost::python::throw_error_already_set();
            }

            const std::vector<UsdGeomXformOp> xformOps = extract<std::vector<UsdGeomXformOp>>(args[0]);

            if (!xformOps.empty())
            {
                for (ssize_t i=1; i < len(args); ++i)
                {
                    const PxrUsdMayaXformStack& stack = extract<PxrUsdMayaXformStack&>(args[i]);
                    object result = MatchingSubstack(stack, xformOps, returnRotOrder);

                    // Need to check if result was successful... slightly trickier since we may get
                    // back a tuple or just the list
                    bool wasEmpty;
                    if(returnRotOrder)
                    {
                        boost::python::tuple& result_tuple = static_cast<boost::python::tuple&>(result);
                        wasEmpty = boost::python::len(result_tuple[0]) == 0;
                    }
                    else
                    {
                        wasEmpty = boost::python::len(result) == 0;
                    }
                    if (!wasEmpty)
                    {
                        return result;
                    }
                }
            }

            if (returnRotOrder)
            {
                return boost::python::make_tuple(
                        boost::python::list(),
                        static_cast<int>(MEulerRotation::kXYZ));
            }
            else
            {
                return boost::python::list();
            }
        }
    };
}



void wrapXformStack()
{
    class_<PxrUsdMayaXformOpClassification,
           PxrUsdMayaXformOpClassificationPtr,
           boost::noncopyable>("XformOpClassification", no_init)
        .def(TfPyWeakPtr())
        .def(self == self)
        .def("GetName", &PxrUsdMayaXformOpClassification::GetName,
                return_value_policy<return_by_value>())
        .def("GetOpType", &Usd_PyXformOpClassification::GetOpType)
        .def("IsInvertedTwin", &PxrUsdMayaXformOpClassification::IsInvertedTwin)
        .def("IsCompatibleType", &PxrUsdMayaXformOpClassification::IsCompatibleType)
        .def("CompatibleAttrNames", &PxrUsdMayaXformOpClassification::CompatibleAttrNames)
        ;

    boost::python::to_python_converter<PxrUsdMayaXformOpClassification,
            Usd_PyXformOpClassification>();

    class_<PxrUsdMayaXformStack>("XformStack", no_init)
        .def("GetOps", &PxrUsdMayaXformStack::GetOps,
                return_value_policy<TfPySequenceToList>())
        .def("GetInversionTwins", &Usd_PyXformStack::GetInversionTwins)
        .def("GetNameMatters", &PxrUsdMayaXformStack::GetNameMatters)
        .def("__getitem__", &Usd_PyXformStack::getitem,
                return_value_policy<reference_existing_object>())
        .def("__len__", &PxrUsdMayaXformStack::GetSize)
        .def("GetSize", &PxrUsdMayaXformStack::GetSize)
        .def("FindOpIndex", &Usd_PyXformStack::FindOpIndex,
                (boost::python::arg("opName"), boost::python::arg("isInvertedTwin")=false))
        .def("FindOp", &PxrUsdMayaXformStack::FindOp,
                (boost::python::arg("opName"), boost::python::arg("isInvertedTwin")=false))
        .def("FindOpIndexPair", &Usd_PyXformStack::FindOpIndexPair)
        .def("FindOpPair", &PxrUsdMayaXformStack::FindOpPair)
        .def("MatchingSubstack", &Usd_PyXformStack::MatchingSubstack,
                (boost::python::arg("xformops"), boost::python::arg("returnRotOrder")=false),
                MATCHING_SUBSTACK_DOCSTRING.c_str())
        .def("MayaStack", &PxrUsdMayaXformStack::MayaStack,
                return_value_policy<reference_existing_object>())
        .staticmethod("MayaStack")
        .def("CommonStack", &PxrUsdMayaXformStack::CommonStack,
                return_value_policy<reference_existing_object>())
        .staticmethod("CommonStack")
        .def("MatrixStack", &PxrUsdMayaXformStack::MatrixStack,
                return_value_policy<reference_existing_object>())
        .staticmethod("MatrixStack")
        .def("FirstMatchingSubstack",
                boost::python::raw_function(&Usd_PyXformStack::FirstMatchingSubstack),
                FIRST_MATCHING_DOCSTRING.c_str())
        .staticmethod("FirstMatchingSubstack")
    ;

    boost::python::to_python_converter<PxrUsdMayaXformStack::OpClassPtrPair,
            Usd_PyXformStack>();
}
