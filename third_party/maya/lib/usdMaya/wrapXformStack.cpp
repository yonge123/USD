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
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/stringUtils.h"

#include <maya/MEulerRotation.h>
#include <maya/MTransformationMatrix.h>

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
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
using boost::python::copy_const_reference;
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
            "The first arg must be a list of PxrUsdMayaXformStack objects to test against,\n"
            "in order, the second arg is the list of UsdGeomXformOps to match.\n"
            "\n"
            "%s",
            RETURN_ROT_ORDER_DOCSTRING.c_str());


    // We wrap this class, instead of PxrUsdMayaXformOpClassification directly, mostly
    // just so we can handle the .IsNull() -> None conversion
    class Usd_PyXformOpClassification {
    public:
        Usd_PyXformOpClassification(const PxrUsdMayaXformOpClassification& opClass)
            : _opClass(opClass)
        {
        }

        bool operator == (const Usd_PyXformOpClassification& other)
        {
            return _opClass == other._opClass;
        }

        TfToken const &GetName() const
        {
            return _opClass.GetName();
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

        UsdGeomXformOp::Type GetOpType() const
        {
            ImportUsdGeomOnce();
            return _opClass.GetOpType();
        }

        bool IsInvertedTwin() const
        {
            return _opClass.IsInvertedTwin();
        }

        bool IsCompatibleType(UsdGeomXformOp::Type otherType) const
        {
            return _opClass.IsCompatibleType(otherType);
        }

        std::vector<TfToken> CompatibleAttrNames() const
        {
            return _opClass.CompatibleAttrNames();
        }

        // to-python conversion of const PxrUsdMayaXformOpClassification.
        static PyObject*
        convert(const PxrUsdMayaXformOpClassification& opClass) {
            TfPyLock lock;
            if (opClass.IsNull())
            {
                return boost::python::incref(Py_None);
            }
            else
            {
                object obj((Usd_PyXformOpClassification(opClass)));
                // Incref because ~object does a decref
                return boost::python::incref(obj.ptr());
            }
        }

    private:
        PxrUsdMayaXformOpClassification _opClass;
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
        convert(const PxrUsdMayaXformStack::OpClassPair& opPair)
        {
            boost::python::tuple result = boost::python::make_tuple(
                    opPair.first, opPair.second);
            // Incref because ~object does a decref
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
        AddOptionalRotOrderToOpMatches(
                std::vector<PxrUsdMayaXformStack::OpClass>& result,
                MTransformationMatrix::RotationOrder rotOrder,
                bool returnRotOrder)
        {
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
        MatchingSubstack(
                const PxrUsdMayaXformStack& stack,
                const std::vector<UsdGeomXformOp>& xformops,
                bool returnRotOrder)
        {
            MTransformationMatrix::RotationOrder rotOrder = MTransformationMatrix::kXYZ;
            std::vector<PxrUsdMayaXformStack::OpClass> result = \
                    stack.MatchingSubstack(xformops, &rotOrder);
            return AddOptionalRotOrderToOpMatches(result, rotOrder, returnRotOrder);
        }

        static object
        FirstMatchingSubstack(
                const std::vector<PxrUsdMayaXformStack const *>& stacks,
                const std::vector<UsdGeomXformOp>& xformops,
                bool returnRotOrder)
        {
            MTransformationMatrix::RotationOrder rotOrder = MTransformationMatrix::kXYZ;
            std::vector<PxrUsdMayaXformStack::OpClass> result = \
                    PxrUsdMayaXformStack::FirstMatchingSubstack(stacks, xformops, &rotOrder);
            return AddOptionalRotOrderToOpMatches(result, rotOrder, returnRotOrder);
        }
    };
}



void wrapXformStack()
{
    class_<Usd_PyXformOpClassification>("XformOpClassification", no_init)
        .def(self == self)
        .def("GetName", &Usd_PyXformOpClassification::GetName,
                return_value_policy<return_by_value>())
        .def("GetOpType", &Usd_PyXformOpClassification::GetOpType)
        .def("IsInvertedTwin", &Usd_PyXformOpClassification::IsInvertedTwin)
        .def("IsCompatibleType", &Usd_PyXformOpClassification::IsCompatibleType)
        .def("CompatibleAttrNames", &Usd_PyXformOpClassification::CompatibleAttrNames)
        ;

    boost::python::to_python_converter<PxrUsdMayaXformOpClassification,
            Usd_PyXformOpClassification>();

    class_<PxrUsdMayaXformStack, boost::noncopyable>("XformStack", no_init)
        .def("GetOps", &PxrUsdMayaXformStack::GetOps,
                return_value_policy<TfPySequenceToList>())
        .def("GetInversionTwins", &Usd_PyXformStack::GetInversionTwins)
        .def("GetNameMatters", &PxrUsdMayaXformStack::GetNameMatters)
        .def("__getitem__", &Usd_PyXformStack::getitem,
                return_value_policy<return_by_value>())
        .def("__len__", &PxrUsdMayaXformStack::GetSize)
        .def("GetSize", &PxrUsdMayaXformStack::GetSize)
        .def("FindOpIndex", &Usd_PyXformStack::FindOpIndex,
                (boost::python::arg("opName"), boost::python::arg("isInvertedTwin")=false))
        .def("FindOp", &PxrUsdMayaXformStack::FindOp,
                (boost::python::arg("opName"), boost::python::arg("isInvertedTwin")=false),
                return_value_policy<copy_const_reference>())
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
        .def("FirstMatchingSubstack", &Usd_PyXformStack::FirstMatchingSubstack,
                (boost::python::arg("stacks"), boost::python::arg("xformops"),
                        boost::python::arg("returnRotOrder")=false),
                FIRST_MATCHING_DOCSTRING.c_str())
        .staticmethod("FirstMatchingSubstack")
    ;

    boost::python::to_python_converter<PxrUsdMayaXformStack::OpClassPair,
            Usd_PyXformStack>();

    TfPyContainerConversions::from_python_sequence<std::vector<PxrUsdMayaXformStack const *>,
        TfPyContainerConversions::variable_capacity_policy >();
}
