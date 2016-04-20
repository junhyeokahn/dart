/*
 * Copyright (c) 2016, Georgia Tech Research Corporation
 * All rights reserved.
 *
 * Author(s): Jeongseok Lee <jslee02@gmail.com>
 *
 * Georgia Tech Graphics Lab and Humanoid Robotics Lab
 *
 * Directed by Prof. C. Karen Liu and Prof. Mike Stilman
 * <karenliu@cc.gatech.edu> <mstilman@cc.gatech.edu>
 *
 * This file is provided under the following "BSD-style" License:
 *   Redistribution and use in source and binary forms, with or
 *   without modification, are permitted provided that the following
 *   conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *   INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 *   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 *   USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 *   AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *   POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DART_MATH_CONFIGURATIONSPACE_H_
#define DART_MATH_CONFIGURATIONSPACE_H_

#include <Eigen/Dense>
#include "dart/math/MathTypes.h"
#include "dart/math/Geometry.h"

namespace dart {
namespace math {

//==============================================================================
template <std::size_t Dimension>
struct RealVectorSpace
{
  enum : std::size_t { NumDofs = Dimension };
  enum : int { NumDofsEigen = Dimension };

  using TangentSpace = RealVectorSpace<NumDofs>;

  using Point          = Eigen::Matrix<double, NumDofs, 1>;
  using EuclideanPoint = Eigen::Matrix<double, NumDofs, 1>;
  using Vector         = Eigen::Matrix<double, NumDofs, 1>;
  using Matrix         = Eigen::Matrix<double, NumDofs, NumDofs>;
  using JacobianMatrix = Eigen::Matrix<double, 6, NumDofs>;
};

using RealSpace = RealVectorSpace<1u>;
using RealVector2Space = RealVectorSpace<2u>;
using RealVector3Space = RealVectorSpace<3u>;

//==============================================================================
struct SO3Space
{
  enum : std::size_t { NumDofs = 3u };
  enum : int { NumDofsEigen = 3 };

  using TangentSpace = RealVectorSpace<NumDofs>;

  using Point          = Eigen::Matrix3d;
  using EuclideanPoint = Eigen::Vector3d;
  using Vector         = Eigen::Vector3d;
  using Matrix         = Eigen::Matrix3d;
  using JacobianMatrix = Eigen::Matrix<double, 6, NumDofs>;
};

//==============================================================================
struct SE3Space
{
  enum : std::size_t { NumDofs = 6u };
  enum : int { NumDofsEigen = 6 };

  using TangentSpace = RealVectorSpace<NumDofs>;

  using Point          = Eigen::Isometry3d;
  using EuclideanPoint = Eigen::Vector6d;
  using Vector         = Eigen::Vector6d;
  using Matrix         = Eigen::Matrix6d;
  using JacobianMatrix = Eigen::Matrix6d;
};

struct MapsToManifoldPoint {};


//==============================================================================
template <typename SpaceT>
typename SpaceT::Matrix inverse(const typename SpaceT::Matrix& mat);

//==============================================================================
template <typename SpaceT>
typename SpaceT::EuclideanPoint
toEuclideanPoint(const typename SpaceT::Point& point);

//==============================================================================
template <typename SpaceT>
typename SpaceT::Point
toManifoldPoint(const typename SpaceT::EuclideanPoint& point);

//==============================================================================
template <typename SpaceT>
typename SpaceT::Point integratePosition(
    const typename SpaceT::Point& pos,
    const typename SpaceT::Vector& vel,
    double dt);

//==============================================================================
template <typename SpaceT>
typename SpaceT::Vector integrateVelocity(
    const typename SpaceT::Vector& vel,
    const typename SpaceT::Vector& acc,
    double dt);

} // namespace math
} // namespace dart

#include "dart/math/detail/ConfigurationSpace.h"

#endif // DART_MATH_CONFIGURATIONSPACE_H_