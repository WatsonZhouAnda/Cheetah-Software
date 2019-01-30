/*! @file DynamicsSimulator.h
 *  @brief Rigid Body Dynamics Simulator with Collisions
 *
 *  Combines ABA, Collisions, integrator, and any other external forces to run a simulation.
 *  Doesn't do any graphics.
 */

#ifndef PROJECT_DYNAMICSSIMULATOR_H
#define PROJECT_DYNAMICSSIMULATOR_H

#include "orientation_tools.h"
#include "FloatingBaseModel.h"
#include "spatial.h"
#include "CollisionPlane.h"
#include "CollisionBox.h"
#include "ContactConstraint.h"

using namespace ori;
using namespace spatial;
#include <eigen3/Eigen/Dense>


/*!
 * Class (containing state) for dynamics simulation of a floating-base system
 */
template<typename T>
class DynamicsSimulator {
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW
      DynamicsSimulator(FloatingBaseModel<T>& model); //! Initialize simulator with given model
  void step(T dt, const DVec<T>& tau, T kp, T kd); //! Simulate forward one step
  
  //! Find _dstate with the articulated body algorithm
  void runABA(const DVec<T>& tau){ _model.runABA(tau, _dstate); }

  //! Do forward kinematics for feet
  void forwardKinematics(){ _model.forwardKinematics(); }
  
  //! Integrate to find new _state
  void integrate(T dt); 

  /*!
   * Set the state of the robot being simulated
   */
  void setState(const FBModelState<T>& state) {
    _state = state;
    _model._state = state;
  }

  /*!
   * Get the state of the robot
   * @return
   */
  const FBModelState<T>& getState() const {
    return _state;
  }

  /*!
   * Get the most recently calculated state derivative (updated by runABA)
   * @return
   */
  const FBModelStateDerivative<T>& getDState() const {
    return _dstate;
  }

  /*!
   * Add external forces. These are added on top of the ground reaction forces
   * The i-th force is the spatial force applied to body i.
   * This is cleared after each step of the simulator.
   */
  void setAllExternalForces(const vectorAligned<SVec<T>>& forces) {
    _model._externalForces = forces;
  }

  // ! Add a collision plane.
  void addCollisionPlane(T mu, T rest, T height) {
    _contact_constr->AddCollision(new CollisionPlane<T>(mu, rest, height) );
  }

  // ! Add a collision box
  void addCollisionBox(T mu, T rest, T depth, T width, T height,
          const Vec3<T> & pos, const Mat3<T> & ori) {
    _contact_constr->AddCollision(
            new CollisionBox<T>(mu, rest, depth, width, height, pos, ori) );
  }

  size_t getNumBodies() {
    return _model._nDof;
  }

  const size_t & getTotalNumGC(){ return _model._nGroundContact; }
  const Vec3<T> & getContactForce(size_t idx){ 
      return _contact_constr->getGCForce(idx); 
  }

  const FloatingBaseModel<T>& getModel(){ return _model; }
private:
  void updateCollisions(T dt, T kp, T kd); //! Update ground collision list

  FBModelState<T> _state;
  FBModelStateDerivative<T> _dstate;
  FloatingBaseModel<T>& _model;
  vector<CollisionPlane<T>> _collisionPlanes;
  ContactConstraint<T> * _contact_constr;
};


#endif //PROJECT_DYNAMICSSIMULATOR_H
