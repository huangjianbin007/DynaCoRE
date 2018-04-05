#include "Mercury_StateProvider.hpp"
#include <Utils/DataManager.hpp>
#include <Mercury/Mercury_Definition.h>

Mercury_StateProvider* Mercury_StateProvider::getStateProvider(){
    static Mercury_StateProvider state_provider_;
    return &state_provider_;
}

Mercury_StateProvider::Mercury_StateProvider(): initialized_(false),
                                system_count_(0),
                                stance_foot_(mercury_link::leftFoot),
                                Q_(mercury::num_q),
                                Qdot_(mercury::num_qdot),
                                reaction_forces_(6),
                                jpos_des_(mercury::num_act_joint),
                                jvel_des_(mercury::num_act_joint),
                                estimated_com_state_(4)
{
  Q_.setZero();
  Qdot_.setZero();
  reaction_forces_.setZero();
  global_pos_local_.setZero();
  des_location_.setZero();
  estimated_com_state_.setZero();

  Rfoot_pos_.setZero();
  Lfoot_pos_.setZero();
  Rfoot_vel_.setZero();
  Lfoot_vel_.setZero();
  foot_pos_des_.setZero();
  foot_vel_des_.setZero();

  CoM_pos_.setZero();
  CoM_vel_.setZero();
  com_pos_des_.setZero();
  com_vel_des_.setZero();

  body_ori_rpy_.setZero();
  body_ang_vel_.setZero();
  body_ang_vel_des_.setZero();

  DataManager* data_manager = DataManager::GetDataManager();

  data_manager->RegisterData(&curr_time_, DOUBLE, "time");
  data_manager->RegisterData(&Q_, DYN_VEC, "config", mercury::num_q);
  data_manager->RegisterData(&Qdot_, DYN_VEC, "qdot", mercury::num_qdot);
  data_manager->RegisterData(&reaction_forces_, DYN_VEC, "reaction_force", 6);

  data_manager->RegisterData(&Rfoot_pos_, VECT3, "rfoot_pos", 3);
  data_manager->RegisterData(&Rfoot_vel_, VECT3, "rfoot_vel", 3);
  data_manager->RegisterData(&Lfoot_pos_, VECT3, "lfoot_pos", 3);
  data_manager->RegisterData(&Lfoot_vel_, VECT3, "lfoot_vel", 3);
  data_manager->RegisterData(&foot_pos_des_, VECT3, "foot_pos_des", 3);
  data_manager->RegisterData(&foot_vel_des_, VECT3, "foot_vel_des", 3);

  data_manager->RegisterData(&CoM_pos_, VECT3, "com_pos", 3);
  data_manager->RegisterData(&CoM_vel_, VECT3, "com_vel", 3);
  data_manager->RegisterData(&com_pos_des_, VECT3, "com_pos_des", 3);
  data_manager->RegisterData(&com_vel_des_, VECT3, "com_vel_des", 3);
  data_manager->RegisterData(&global_pos_local_, VECT3, "global_pos_local", 3);


  data_manager->RegisterData(&imu_acc_inc_, VECT3, "imu_acc_inc", 3);
  data_manager->RegisterData(&imu_acc_, VECT3, "imu_acc", 3);
  data_manager->RegisterData(&imu_ang_vel_, VECT3, "imu_ang_vel", 3);

  data_manager->RegisterData(&body_ori_, QUATERNION, "body_ori", 4);
  data_manager->RegisterData(&body_ori_rpy_, VECT3, "body_ori_rpy", 3);
  data_manager->RegisterData(&body_ori_des_, QUATERNION, "body_ori_des", 4);
  data_manager->RegisterData(&body_ang_vel_des_, VECT3, "body_ang_vel_des", 3);
  data_manager->RegisterData(&body_ang_vel_, VECT3, "body_ang_vel", 3);

  data_manager->RegisterData(&jpos_des_, DYN_VEC, "jpos_des", mercury::num_act_joint);
  data_manager->RegisterData(&jvel_des_, DYN_VEC, "jvel_des", mercury::num_act_joint);
  data_manager->RegisterData(&estimated_com_state_, DYN_VEC, "estimated_com_state", 4);
}


void Mercury_StateProvider::SaveCurrentData(){

  double yaw, pitch, roll;
  dynacore::convert(body_ori_, yaw, pitch, roll);
  body_ori_rpy_[0] = roll;
  body_ori_rpy_[1] = pitch;
  body_ori_rpy_[2] = yaw;
}