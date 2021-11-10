// Copyright 2019 Alexander Liniger

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#ifndef MPCC_COST_H
#define MPCC_COST_H

#include "config.h"
#include "types.h"
#include "Spline/arc_length_spline.h"

namespace mpcc{
struct CostMatrix{
    Q_MPC Q;
    R_MPC R;
    S_MPC S;
    q_MPC q;
    r_MPC r;
    Z_MPC Z;
    z_MPC z;
};

struct TrackPoint{
    const double x_ref;
    const double y_ref;
    const double dx_ref;
    const double dy_ref;
    const double theta_ref;
    const double dtheta_ref;
};

struct ErrorInfo{
    const Eigen::Matrix<double,1,2> error;
    const Eigen::Matrix<double,2,NX> d_error;
};

class Cost {
public:
    CostMatrix getCost(const ArcLengthSpline &track, const State &x, const Input &u,int k) const;
    CostMatrix getCost(const int num_of_opponents, const std::vector<ArcLengthSpline> &tracks, const std::vector<Eigen::VectorXd> considering_paths_phi, const ArcLengthSpline &track, const State &x, const Input &u,const int k) const;

    Cost(const PathToJson &path);
    Cost();
    CostParam cost_param_;
    double Ts_=0.01;

private:
    TrackPoint getRefPoint(const ArcLengthSpline &track,const State &x) const;
    ErrorInfo  getErrorInfo(const ArcLengthSpline &track,const State &x) const;
    ErrorInfo  calcPredDist(const ArcLengthSpline &track, const State &x, const int k) const;
    ErrorInfo  calcPotentialField(const ArcLengthSpline &track, const Eigen::VectorXd &considering_path_phi, const State &x, const int k) const;
    ErrorInfo  calcProbabilityDensity(const ArcLengthSpline &track, const Eigen::VectorXd considering_path_phi, const State &x, const int k) const;

    CostMatrix getContouringCost(const ArcLengthSpline &track, const State &x,int k) const;
    CostMatrix getMinusContouringCost(const ArcLengthSpline &track, const Eigen::VectorXd considering_path_phi, const State &x,const int k) const;
    CostMatrix getHeadingCost(const ArcLengthSpline &track, const State &x,int k) const;
    CostMatrix getCollisionCost(const std::vector<ArcLengthSpline> &tracks, const std::vector<Eigen::VectorXd> considering_paths_phi, const State &x,int k) const; // tracks: opponent index, X/Y, timestep
    CostMatrix getInputCost() const;
    CostMatrix getBetaCost(const State &x) const;
    CostMatrix getBetaKinCost(const State &x) const;
    CostMatrix getSoftConstraintCost() const;

    Param param_;
};
}
#endif //MPCC_COST_H
