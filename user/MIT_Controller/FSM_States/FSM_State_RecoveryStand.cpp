/*============================= Recovery Stand ==============================*/
/**
 * Transitionary state that is called for the robot to stand up into
 * balance control mode.
 */

#include "FSM_State_RecoveryStand.h"
#include <Utilities/Utilities_print.h>


/**
 * Constructor for the FSM State that passes in state specific info to
 * the generic FSM State constructor.
 *
 * @param _controlFSMData holds all of the relevant control data
 */
template <typename T>
FSM_State_RecoveryStand<T>::FSM_State_RecoveryStand(ControlFSMData<T>* _controlFSMData)
    : FSM_State<T>(_controlFSMData, FSM_StateName::STAND_UP, "STAND_UP"){
  // Do nothing
  // Set the pre controls safety checks
  this->checkSafeOrientation = false;

  // Post control safety checks
  this->checkPDesFoot = false;
  this->checkForceFeedForward = false;

  zero_vec3.setZero();
  // goal configuration
  // Folding
  fold_jpos[0] << -0.0f, -1.4f, 2.7f;
  fold_jpos[1] << 0.0f, -1.4f, 2.7f;
  fold_jpos[2] << -0.0f, -1.4f, 2.7f;
  fold_jpos[3] << 0.0f, -1.4f, 2.7f;
  // Stand Up
  for(size_t i(0); i<4; ++i){
    stand_jpos[i] << 0.f, -.9425f, 1.885f;
  }
  // Rolling
  rolling_jpos[0] << 1.5f, -1.6f, 2.77f;
  rolling_jpos[1] << 1.3f, -3.1f, 2.77f;
  rolling_jpos[2] << 1.5f, -1.6f, 2.77f;
  rolling_jpos[3] << 1.3f, -3.1f, 2.77f;

  f_ff << 0.f, 0.f, -25.f;
}

template <typename T>
void FSM_State_RecoveryStand<T>::onEnter() {
  // Default is to not transition
  this->nextStateName = this->stateName;

  // Reset the transition data
  this->transitionData.zero();

  // Reset iteration counter
  iter = 0;
  
  // initial configuration, position
  for(size_t i(0); i < 4; ++i) {
    initial_jpos[i] = this->_data->_legController->datas[i].q;
  }

  T body_height = 
    this->_data->_stateEstimator->getResult().position[2];

  _flag = FoldLegs;
  if( !_UpsideDown() ) { // Proper orientation
    if (  (0.2 < body_height) && (body_height < 0.3) ){
      _flag = StandUp;
    }
  }
  _motion_start_iter = 0;
}

template <typename T>
bool FSM_State_RecoveryStand<T>::_UpsideDown(){
  if(this->_data->_stateEstimator->getResult().aBody[2] < 0){
    return true;
  }
  return false;
}

/**
 * Calls the functions to be executed on each control loop iteration.
 */
template <typename T>
void FSM_State_RecoveryStand<T>::run() {

  switch(_flag){
    case StandUp:
      _StandUp(iter - _motion_start_iter);
      break;
    case FoldLegs:
      _FoldLegs(iter - _motion_start_iter);
      break;
    case RollOver:
      _RollOver(iter - _motion_start_iter);
      break;
  }

}

template <typename T>
void FSM_State_RecoveryStand<T>::_SetJPosInterPts(
    const size_t & curr_iter, size_t max_iter, int leg, 
    const Vec3<T> & ini, const Vec3<T> & fin){

    float a(0.f);
    float b(1.f);

    // if we're done interpolating
    if(curr_iter < max_iter) {
      b = (float)curr_iter/(float)max_iter;
      a = 1.f - b;
    }

    // compute setpoints
    Vec3<T> inter_pos = a * ini + b * fin;

    //pretty_print(inter_pos, std::cout, "inter pos");
    // do control
    this->jointPDControl(leg, inter_pos, zero_vec3);
}

template <typename T>
void FSM_State_RecoveryStand<T>::_RollOver(const int & curr_iter){

  for(size_t i(0); i<4; ++i){
    _SetJPosInterPts(curr_iter, rollover_ramp_iter, i, 
        fold_jpos[i], rolling_jpos[i]);
  }

  if(curr_iter > rollover_ramp_iter + rollover_settle_iter){
    _flag = FoldLegs;
    for(size_t i(0); i<4; ++i) initial_jpos[i] = rolling_jpos[i];
    _motion_start_iter = iter;
  }
}

template <typename T>
void FSM_State_RecoveryStand<T>::_StandUp(const int & curr_iter){
  for(size_t leg(0); leg<4; ++leg){
    _SetJPosInterPts(curr_iter, standup_ramp_iter, 
        leg, initial_jpos[leg], stand_jpos[leg]);
  }
  // feed forward mass of robot.
  //for(int i = 0; i < 4; i++)
    //this->_data->_legController->commands[i].forceFeedForward = f_ff;
  //Vec4<T> se_contactState(0.,0.,0.,0.);
  Vec4<T> se_contactState(0.5,0.5,0.5,0.5);
  this->_data->_stateEstimator->setContactPhase(se_contactState);
}

template <typename T>
void FSM_State_RecoveryStand<T>::_FoldLegs(const int & curr_iter){

  for(size_t i(0); i<4; ++i){
    _SetJPosInterPts(curr_iter, fold_ramp_iter, i, 
        initial_jpos[i], fold_jpos[i]);
  }
  if(curr_iter > fold_ramp_iter + fold_settle_iter){
    if(_UpsideDown()){
      _flag = RollOver;
    }else{
      _flag = StandUp;
      for(size_t i(0); i<4; ++i) initial_jpos[i] = fold_jpos[i];
    }
    _motion_start_iter = iter;
  }
}

/**
 * Manages which states can be transitioned into either by the user
 * commands or state event triggers.
 *
 * @return the enumerated FSM state name to transition into
 */
template <typename T>
FSM_StateName FSM_State_RecoveryStand<T>::checkTransition() {
  this->nextStateName = this->stateName;
  iter++;

  // Switch FSM control mode
  switch ((int)this->_data->controlParameters->control_mode) {
    case K_RECOVERY_STAND:
      break;

    case K_LOCOMOTION:
      this->nextStateName = FSM_StateName::LOCOMOTION;
      break;

    case K_BOUNDING:
      this->nextStateName = FSM_StateName::BOUNDING;
      break;


    case K_PASSIVE:  // normal c
      this->nextStateName = FSM_StateName::PASSIVE;
      break;

    case K_BALANCE_STAND: 
      this->nextStateName = FSM_StateName::BALANCE_STAND;
      break;

    default:
      std::cout << "[CONTROL FSM] Bad Request: Cannot transition from "
                << K_RECOVERY_STAND << " to "
                << this->_data->controlParameters->control_mode << std::endl;
  }

  // Get the next state
  return this->nextStateName;
}

/**
 * Handles the actual transition for the robot between states.
 * Returns true when the transition is completed.
 *
 * @return true if transition is complete
 */
template <typename T>
TransitionData<T> FSM_State_RecoveryStand<T>::transition() {
  // Finish Transition
  switch (this->nextStateName) {
    case FSM_StateName::PASSIVE:  // normal
      this->transitionData.done = true;
      break;

    case FSM_StateName::BALANCE_STAND:
      this->transitionData.done = true;
      break;

    case FSM_StateName::LOCOMOTION:
      this->transitionData.done = true;
      break;

    case FSM_StateName::BOUNDING:
      this->transitionData.done = true;
      break;


    default:
      std::cout << "[CONTROL FSM] Something went wrong in transition"
                << std::endl;
  }

  // Return the transition data to the FSM
  return this->transitionData;
}

/**
 * Cleans up the state information on exiting the state.
 */
template <typename T>
void FSM_State_RecoveryStand<T>::onExit() {
  // Nothing to clean up when exiting
}

// template class FSM_State_RecoveryStand<double>;
template class FSM_State_RecoveryStand<float>;
