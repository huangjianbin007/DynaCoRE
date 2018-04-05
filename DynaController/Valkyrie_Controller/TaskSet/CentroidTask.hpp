#ifndef VALKYRIE_WBC_CENTROID_TASK
#define VALKYRIE_WBC_CENTROID_TASK

#include <WBC/Task.hpp>

class Valkyrie_StateProvider;
class RobotSystem;

class CentroidTask: public Task{
public:
  CentroidTask(RobotSystem* );
  virtual ~CentroidTask();

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
    // no additional update
  virtual bool _AdditionalUpdate(){ return true;}

  const Valkyrie_StateProvider* sp_;
  const RobotSystem* robot_sys_;
};

#endif