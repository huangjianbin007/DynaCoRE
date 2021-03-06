#ifndef SagitP3_State_INTERFACE_H
#define SagitP3_State_INTERFACE_H

#include <Utils/wrap_eigen.hpp>
#include <Configuration.h>
#include <interface.hpp>
#include "SagitP3_DynaCtrl_Definition.h"
#include <Filter/filters.hpp>

class SagitP3_StateEstimator;
class SagitP3_StateProvider;

class SagitP3_state_interface: public interface{
public:
  SagitP3_state_interface();
  virtual ~SagitP3_state_interface();
  virtual void GetCommand(void * data, void * command);

private:
  int waiting_count_;
  
  void _ParameterSetting();

  SagitP3_Command* test_cmd_;
 
  dynacore::Vector torque_command_;
  dynacore::Vector jpos_command_;
  dynacore::Vector jvel_command_;
  
  SagitP3_StateProvider* sp_;
};

#endif
