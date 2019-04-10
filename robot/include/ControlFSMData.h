#ifndef CONTROLFSMDATA_H
#define CONTROLFSMDATA_H

//#include <iostream>
//#include "RobotController.h"
//#include "FSM_States/FSM_State.h"
#include "Controllers/StateEstimatorContainer.h"
#include "Controllers/LegController.h"
#include "Controllers/GaitScheduler.h"
#include "Controllers/DesiredStateCommand.h"




/*
 *
 */
template <typename T>
struct ControlFSMData {
  //EIGEN_MAKE_ALIGNED_OPERATOR_NEW
  StateEstimatorContainer<T>* _stateEstimator;
  LegController<T>* _legController;
  GaitScheduler<T>* _gaitScheduler;
  DesiredStateCommand<T>* _desiredStateCommand;
};


template struct ControlFSMData<double>;
template struct ControlFSMData<float>;


#endif // CONTROLFSM_H