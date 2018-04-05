#ifndef LINK_XYZ_TASK
#define LINK_XYZ_TASK

#include <WBDC/WBDC_Task.hpp>

class Mercury_StateProvider;
class RobotSystem;

class LinkXYZTask: public WBDC_Task{
public:
  LinkXYZTask(RobotSystem*, int link_id);
  virtual ~LinkXYZTask();

  dynacore::Vector Kp_vec_;
  dynacore::Vector Kd_vec_;

protected:
  // Update op_cmd_
  virtual bool _UpdateCommand(void* pos_des,
                              const dynacore::Vector & vel_des,
                              const dynacore::Vector & acc_des);
  // Update Jt_
  virtual bool _UpdateTaskJacobian();
  // Update JtDotQdot_
  virtual bool _UpdateTaskJDotQdot();

  int link_id_;
  Mercury_StateProvider* sp_;
  RobotSystem* robot_sys_;
};

#endif