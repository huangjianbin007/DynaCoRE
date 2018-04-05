#ifndef STATE_PROVIDER_MERCURY
#define STATE_PROVIDER_MERCURY

#include <Utils/utilities.hpp>
#include <Utils/wrap_eigen.hpp>
#include <Configuration.h>

using namespace dynacore;

class Mercury_StateProvider{
public:
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  static Mercury_StateProvider* getStateProvider();
  ~Mercury_StateProvider(){}

  dynacore::Quaternion body_ori_;
  dynacore::Vect3 body_ori_rpy_;
  dynacore::Quaternion body_ori_des_;
  dynacore::Vect3 body_ang_vel_;
  dynacore::Vect3 body_ang_vel_des_;
  dynacore::Vect3 imu_acc_inc_;
  dynacore::Vect3 imu_ang_vel_;
  dynacore::Vect3 imu_acc_;
  // Important!!!!!!!!
  int stance_foot_;

  bool initialized_;
  double curr_time_;
  int system_count_;

  Vector Q_;
  Vector Qdot_;
  Vector curr_torque_;

  dynacore::Vector reaction_forces_;

  dynacore::Vect3 global_pos_local_;
  dynacore::Vect2 des_location_;

  dynacore::Vect3 Rfoot_pos_;
  dynacore::Vect3 Lfoot_pos_;
  dynacore::Vect3 Rfoot_vel_;
  dynacore::Vect3 Lfoot_vel_;
  dynacore::Vect3 foot_pos_des_;
  dynacore::Vect3 foot_vel_des_;


  dynacore::Vect3 CoM_pos_;
  dynacore::Vect3 CoM_vel_;
  dynacore::Vect3 com_pos_des_;
  dynacore::Vect3 com_vel_des_;

  dynacore::Vector jpos_des_;
  dynacore::Vector jvel_des_;

  void SaveCurrentData();

  // (x, y, x_dot, y_dot)
  dynacore::Vector estimated_com_state_;
private:
  Mercury_StateProvider();
};


#endif