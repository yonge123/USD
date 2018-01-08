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
////////////////////////////////////////////////////////////////////////
// This file is generated by a script.  Do not edit directly.  Edit the
// matrix3.template.cpp file to make changes.


#include "pxr/pxr.h"
#include "pxr/base/gf/matrix3d.h"
#include "pxr/base/gf/matrix3f.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/ostreamHelpers.h"
#include "pxr/base/tf/type.h"

#include "pxr/base/gf/rotation.h"
#include <float.h>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<GfMatrix3d>();
}

std::ostream&
operator<<(std::ostream& out, const GfMatrix3d& m)
{
    return out
        << "( ("
        << Gf_OstreamHelperP(m[0][0]) << ", "
        << Gf_OstreamHelperP(m[0][1]) << ", "
        << Gf_OstreamHelperP(m[0][2])
        << "), ("
        << Gf_OstreamHelperP(m[1][0]) << ", "
        << Gf_OstreamHelperP(m[1][1]) << ", "
        << Gf_OstreamHelperP(m[1][2])
        << "), ("
        << Gf_OstreamHelperP(m[2][0]) << ", "
        << Gf_OstreamHelperP(m[2][1]) << ", "
        << Gf_OstreamHelperP(m[2][2])
        << ") )";
}

GfMatrix3d::GfMatrix3d(const GfMatrix3f& m)
{
    Set(m[0][0], m[0][1], m[0][2], 
        m[1][0], m[1][1], m[1][2], 
        m[2][0], m[2][1], m[2][2]);
}

GfMatrix3d::GfMatrix3d(const std::vector< std::vector<double> >& v)
{
    double m[3][3] = {{1.0, 0.0, 0.0},
                      {0.0, 1.0, 0.0},
                      {0.0, 0.0, 1.0}};
    for(size_t row = 0; row < 3 && row < v.size(); ++row) {
        for (size_t col = 0; col < 3 && col < v[row].size(); ++col) {
            m[row][col] = v[row][col];
        }
    }
    Set(m);
}

GfMatrix3d::GfMatrix3d(const std::vector< std::vector<float> >& v)
{
    double m[3][3] = {{1.0, 0.0, 0.0},
                      {0.0, 1.0, 0.0},
                      {0.0, 0.0, 1.0}};
    for(size_t row = 0; row < 3 && row < v.size(); ++row) {
        for (size_t col = 0; col < 3 && col < v[row].size(); ++col) {
            m[row][col] = v[row][col];
        }
    }
    Set(m);
}

GfMatrix3d::GfMatrix3d(const GfRotation &rot)
{
    SetRotate(rot);
}

GfMatrix3d &
GfMatrix3d::SetDiagonal(double s)
{
    _mtx[0][0] = s;
    _mtx[0][1] = 0.0;
    _mtx[0][2] = 0.0;
    _mtx[1][0] = 0.0;
    _mtx[1][1] = s;
    _mtx[1][2] = 0.0;
    _mtx[2][0] = 0.0;
    _mtx[2][1] = 0.0;
    _mtx[2][2] = s;
    return *this;
}

GfMatrix3d &
GfMatrix3d::SetDiagonal(const GfVec3d& v)
{
    _mtx[0][0] = v[0]; _mtx[0][1] = 0.0; _mtx[0][2] = 0.0; 
    _mtx[1][0] = 0.0; _mtx[1][1] = v[1]; _mtx[1][2] = 0.0; 
    _mtx[2][0] = 0.0; _mtx[2][1] = 0.0; _mtx[2][2] = v[2];
    return *this;
}

double *
GfMatrix3d::Get(double m[3][3]) const
{
    m[0][0] = _mtx[0][0];
    m[0][1] = _mtx[0][1];
    m[0][2] = _mtx[0][2];
    m[1][0] = _mtx[1][0];
    m[1][1] = _mtx[1][1];
    m[1][2] = _mtx[1][2];
    m[2][0] = _mtx[2][0];
    m[2][1] = _mtx[2][1];
    m[2][2] = _mtx[2][2];
    return &m[0][0];
}

bool
GfMatrix3d::operator ==(const GfMatrix3d &m) const
{
    return (_mtx[0][0] == m._mtx[0][0] &&
            _mtx[0][1] == m._mtx[0][1] &&
            _mtx[0][2] == m._mtx[0][2] &&
            _mtx[1][0] == m._mtx[1][0] &&
            _mtx[1][1] == m._mtx[1][1] &&
            _mtx[1][2] == m._mtx[1][2] &&
            _mtx[2][0] == m._mtx[2][0] &&
            _mtx[2][1] == m._mtx[2][1] &&
            _mtx[2][2] == m._mtx[2][2]);
}

bool
GfMatrix3d::operator ==(const GfMatrix3f &m) const
{
    return (_mtx[0][0] == m._mtx[0][0] &&
            _mtx[0][1] == m._mtx[0][1] &&
            _mtx[0][2] == m._mtx[0][2] &&
            _mtx[1][0] == m._mtx[1][0] &&
            _mtx[1][1] == m._mtx[1][1] &&
            _mtx[1][2] == m._mtx[1][2] &&
            _mtx[2][0] == m._mtx[2][0] &&
            _mtx[2][1] == m._mtx[2][1] &&
            _mtx[2][2] == m._mtx[2][2]);
}


GfMatrix3d
GfMatrix3d::GetTranspose() const
{
    GfMatrix3d transpose;
    transpose._mtx[0][0] = _mtx[0][0];
    transpose._mtx[1][0] = _mtx[0][1];
    transpose._mtx[2][0] = _mtx[0][2];
    transpose._mtx[0][1] = _mtx[1][0];
    transpose._mtx[1][1] = _mtx[1][1];
    transpose._mtx[2][1] = _mtx[1][2];
    transpose._mtx[0][2] = _mtx[2][0];
    transpose._mtx[1][2] = _mtx[2][1];
    transpose._mtx[2][2] = _mtx[2][2];

    return transpose;
}

GfMatrix3d
GfMatrix3d::GetInverse(double *detPtr, double eps) const
{
    double a00,a01,a02,a10,a11,a12,a20,a21,a22;
    double det, rcp;

    a00 = _mtx[0][0];
    a01 = _mtx[0][1];
    a02 = _mtx[0][2];
    a10 = _mtx[1][0];
    a11 = _mtx[1][1];
    a12 = _mtx[1][2];
    a20 = _mtx[2][0];
    a21 = _mtx[2][1];
    a22 = _mtx[2][2];
    det = -(a02*a11*a20) + a01*a12*a20 + a02*a10*a21 - 
	  a00*a12*a21 - a01*a10*a22 + a00*a11*a22;

    if (detPtr) {
	*detPtr = det;
    }

    GfMatrix3d inverse;

    if (GfAbs(det) > eps) {
	rcp = 1.0 / det;
        inverse._mtx[0][0] = (-(a12*a21) + a11*a22)*rcp;
        inverse._mtx[0][1] = (a02*a21 - a01*a22)*rcp;
        inverse._mtx[0][2] = (-(a02*a11) + a01*a12)*rcp;
        inverse._mtx[1][0] = (a12*a20 - a10*a22)*rcp;
        inverse._mtx[1][1] = (-(a02*a20) + a00*a22)*rcp;
        inverse._mtx[1][2] = (a02*a10 - a00*a12)*rcp;
        inverse._mtx[2][0] = (-(a11*a20) + a10*a21)*rcp;
        inverse._mtx[2][1] = (a01*a20 - a00*a21)*rcp;
        inverse._mtx[2][2] = (-(a01*a10) + a00*a11)*rcp;

    }
    else {
       	inverse.SetScale(FLT_MAX);
    }

    return inverse;
}

double
GfMatrix3d::GetDeterminant() const
{
    return (_mtx[0][0] * _mtx[1][1] * _mtx[2][2] +
	    _mtx[0][1] * _mtx[1][2] * _mtx[2][0] +
	    _mtx[0][2] * _mtx[1][0] * _mtx[2][1] -
	    _mtx[0][0] * _mtx[1][2] * _mtx[2][1] -
	    _mtx[0][1] * _mtx[1][0] * _mtx[2][2] -
	    _mtx[0][2] * _mtx[1][1] * _mtx[2][0]);
}

double
GfMatrix3d::GetHandedness() const
{
    // Note: This can be computed with fewer arithmetic operations using a
    //       cross and dot product, but it is more important that the result
    //       is consistent with the way the determinant is computed.
    return GfSgn(GetDeterminant());
}

/* Make the matrix orthonormal in place using an iterative method.
 * It is potentially slower if the matrix is far from orthonormal (i.e. if
 * the row basis vectors are close to colinear) but in the common case
 * of near-orthonormality it should be just as fast. */
bool
GfMatrix3d::Orthonormalize(bool issueWarning)
{
    // orthogonalize and normalize row vectors
    GfVec3d r0(_mtx[0][0],_mtx[0][1],_mtx[0][2]);
    GfVec3d r1(_mtx[1][0],_mtx[1][1],_mtx[1][2]);
    GfVec3d r2(_mtx[2][0],_mtx[2][1],_mtx[2][2]);
    bool result = GfVec3d::OrthogonalizeBasis(&r0, &r1, &r2, true);
    _mtx[0][0] = r0[0];
    _mtx[0][1] = r0[1];
    _mtx[0][2] = r0[2];
    _mtx[1][0] = r1[0];
    _mtx[1][1] = r1[1];
    _mtx[1][2] = r1[2];
    _mtx[2][0] = r2[0];
    _mtx[2][1] = r2[1];
    _mtx[2][2] = r2[2];
    if (!result && issueWarning)
	TF_WARN("OrthogonalizeBasis did not converge, matrix may not be "
                "orthonormal.");
    return result;
}

GfMatrix3d
GfMatrix3d::GetOrthonormalized(bool issueWarning) const
{
    GfMatrix3d result = *this;
    result.Orthonormalize(issueWarning);
    return result;
}

/*
** Scaling
*/
GfMatrix3d&
GfMatrix3d::operator*=(double d)
{
    _mtx[0][0] *= d; _mtx[0][1] *= d; _mtx[0][2] *= d; 
    _mtx[1][0] *= d; _mtx[1][1] *= d; _mtx[1][2] *= d; 
    _mtx[2][0] *= d; _mtx[2][1] *= d; _mtx[2][2] *= d;
    return *this;
}

/*
** Addition
*/
GfMatrix3d &
GfMatrix3d::operator+=(const GfMatrix3d &m)
{
    _mtx[0][0] += m._mtx[0][0];
    _mtx[0][1] += m._mtx[0][1];
    _mtx[0][2] += m._mtx[0][2];
    _mtx[1][0] += m._mtx[1][0];
    _mtx[1][1] += m._mtx[1][1];
    _mtx[1][2] += m._mtx[1][2];
    _mtx[2][0] += m._mtx[2][0];
    _mtx[2][1] += m._mtx[2][1];
    _mtx[2][2] += m._mtx[2][2];
    return *this;
}

/*
** Subtraction
*/
GfMatrix3d &
GfMatrix3d::operator-=(const GfMatrix3d &m)
{
    _mtx[0][0] -= m._mtx[0][0];
    _mtx[0][1] -= m._mtx[0][1];
    _mtx[0][2] -= m._mtx[0][2];
    _mtx[1][0] -= m._mtx[1][0];
    _mtx[1][1] -= m._mtx[1][1];
    _mtx[1][2] -= m._mtx[1][2];
    _mtx[2][0] -= m._mtx[2][0];
    _mtx[2][1] -= m._mtx[2][1];
    _mtx[2][2] -= m._mtx[2][2];
    return *this;
}

/*
** Negation
*/
GfMatrix3d
operator -(const GfMatrix3d& m)
{
    return
        GfMatrix3d(-m._mtx[0][0], -m._mtx[0][1], -m._mtx[0][2], 
                   -m._mtx[1][0], -m._mtx[1][1], -m._mtx[1][2], 
                   -m._mtx[2][0], -m._mtx[2][1], -m._mtx[2][2]);
}

GfMatrix3d &
GfMatrix3d::operator*=(const GfMatrix3d &m)
{
    // Save current values before they are overwritten
    GfMatrix3d tmp = *this;

    _mtx[0][0] = tmp._mtx[0][0] * m._mtx[0][0] +
                 tmp._mtx[0][1] * m._mtx[1][0] +
                 tmp._mtx[0][2] * m._mtx[2][0];

    _mtx[0][1] = tmp._mtx[0][0] * m._mtx[0][1] +
                 tmp._mtx[0][1] * m._mtx[1][1] +
                 tmp._mtx[0][2] * m._mtx[2][1];

    _mtx[0][2] = tmp._mtx[0][0] * m._mtx[0][2] +
                 tmp._mtx[0][1] * m._mtx[1][2] +
                 tmp._mtx[0][2] * m._mtx[2][2];

    _mtx[1][0] = tmp._mtx[1][0] * m._mtx[0][0] +
                 tmp._mtx[1][1] * m._mtx[1][0] +
                 tmp._mtx[1][2] * m._mtx[2][0];

    _mtx[1][1] = tmp._mtx[1][0] * m._mtx[0][1] +
                 tmp._mtx[1][1] * m._mtx[1][1] +
                 tmp._mtx[1][2] * m._mtx[2][1];

    _mtx[1][2] = tmp._mtx[1][0] * m._mtx[0][2] +
                 tmp._mtx[1][1] * m._mtx[1][2] +
                 tmp._mtx[1][2] * m._mtx[2][2];

    _mtx[2][0] = tmp._mtx[2][0] * m._mtx[0][0] +
                 tmp._mtx[2][1] * m._mtx[1][0] +
                 tmp._mtx[2][2] * m._mtx[2][0];

    _mtx[2][1] = tmp._mtx[2][0] * m._mtx[0][1] +
                 tmp._mtx[2][1] * m._mtx[1][1] +
                 tmp._mtx[2][2] * m._mtx[2][1];

    _mtx[2][2] = tmp._mtx[2][0] * m._mtx[0][2] +
                 tmp._mtx[2][1] * m._mtx[1][2] +
                 tmp._mtx[2][2] * m._mtx[2][2];

    return *this;
}

/*
 * Define multiplication between floating vector and double matrix.
 */
GfVec3f
operator *(const GfVec3f &vec, const GfMatrix3d &m)
{
    return GfVec3f(
        float(vec[0] * m._mtx[0][0] + vec[1] * m._mtx[1][0] + vec[2] * m._mtx[2][0]),
        float(vec[0] * m._mtx[0][1] + vec[1] * m._mtx[1][1] + vec[2] * m._mtx[2][1]),
        float(vec[0] * m._mtx[0][2] + vec[1] * m._mtx[1][2] + vec[2] * m._mtx[2][2]));
}

GfVec3f
operator *(const GfMatrix3d& m, const GfVec3f &vec)
{
    return GfVec3f(
        float(vec[0] * m._mtx[0][0] + vec[1] * m._mtx[0][1] + vec[2] * m._mtx[0][2]),
        float(vec[0] * m._mtx[1][0] + vec[1] * m._mtx[1][1] + vec[2] * m._mtx[1][2]),
        float(vec[0] * m._mtx[2][0] + vec[1] * m._mtx[2][1] + vec[2] * m._mtx[2][2]));
}
GfMatrix3d &
GfMatrix3d::SetScale(double s)
{
    _mtx[0][0] = s;   _mtx[0][1] = 0.0; _mtx[0][2] = 0.0;
    _mtx[1][0] = 0.0; _mtx[1][1] = s;   _mtx[1][2] = 0.0;
    _mtx[2][0] = 0.0; _mtx[2][1] = 0.0; _mtx[2][2] = s;

    return *this;
}

GfMatrix3d &
GfMatrix3d::SetRotate(const GfRotation &rot)
{
    GfQuaternion quat = rot.GetQuaternion();

    double  r = quat.GetReal();
    GfVec3d i = quat.GetImaginary();


    _mtx[0][0] = 1.0 - 2.0 * (i[1] * i[1] + i[2] * i[2]);
    _mtx[0][1] =       2.0 * (i[0] * i[1] + i[2] *    r);
    _mtx[0][2] =       2.0 * (i[2] * i[0] - i[1] *    r);

    _mtx[1][0] =       2.0 * (i[0] * i[1] - i[2] *    r);
    _mtx[1][1] = 1.0 - 2.0 * (i[2] * i[2] + i[0] * i[0]);
    _mtx[1][2] =       2.0 * (i[1] * i[2] + i[0] *    r);

    _mtx[2][0] =       2.0 * (i[2] * i[0] + i[1] *    r);
    _mtx[2][1] =       2.0 * (i[1] * i[2] - i[0] *    r);
    _mtx[2][2] = 1.0 - 2.0 * (i[1] * i[1] + i[0] * i[0]);

    return *this;
}

GfMatrix3d &
GfMatrix3d::SetScale(const GfVec3d &s)
{
    _mtx[0][0] = s[0]; _mtx[0][1] = 0.0;  _mtx[0][2] = 0.0;
    _mtx[1][0] = 0.0;  _mtx[1][1] = s[1]; _mtx[1][2] = 0.0;
    _mtx[2][0] = 0.0;  _mtx[2][1] = 0.0;  _mtx[2][2] = s[2];

    return *this;
}

GfQuaternion
GfMatrix3d::ExtractRotationQuaternion() const
{
    // This was adapted from the (open source) Open Inventor
    // SbRotation::SetValue(const SbMatrix &m)

    int i;

    // First, find largest diagonal in matrix:
    if (_mtx[0][0] > _mtx[1][1])
	i = (_mtx[0][0] > _mtx[2][2] ? 0 : 2);
    else
	i = (_mtx[1][1] > _mtx[2][2] ? 1 : 2);

    GfVec3d im;
    double  r;

    if (_mtx[0][0] + _mtx[1][1] + _mtx[2][2] > _mtx[i][i]) {
	r = 0.5 * sqrt(_mtx[0][0] + _mtx[1][1] +
		       _mtx[2][2] + 1);
	im.Set((_mtx[1][2] - _mtx[2][1]) / (4.0 * r),
	       (_mtx[2][0] - _mtx[0][2]) / (4.0 * r),
	       (_mtx[0][1] - _mtx[1][0]) / (4.0 * r));
    }
    else {
	int j = (i + 1) % 3;
	int k = (i + 2) % 3;
	double q = 0.5 * sqrt(_mtx[i][i] - _mtx[j][j] -
			      _mtx[k][k] + 1); 

	im[i] = q;
	im[j] = (_mtx[i][j] + _mtx[j][i]) / (4 * q);
	im[k] = (_mtx[k][i] + _mtx[i][k]) / (4 * q);
	r     = (_mtx[j][k] - _mtx[k][j]) / (4 * q);
    }

    return GfQuaternion(GfClamp(r, -1.0, 1.0), im);
}

GfRotation
GfMatrix3d::ExtractRotation() const
{
    return GfRotation( ExtractRotationQuaternion() );
}

GfVec3d
GfMatrix3d::DecomposeRotation(const GfVec3d &axis0,
                             const GfVec3d &axis1,
                             const GfVec3d &axis2) const
{
    return (ExtractRotation().Decompose(axis0, axis1, axis2));
}

PXR_NAMESPACE_CLOSE_SCOPE
