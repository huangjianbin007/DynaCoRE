#include "BodyFootPlanningCtrl.hpp"
#include <Configuration.h>
#include <Mercury_Controller/Mercury_StateProvider.hpp>
#include <Mercury_Controller/TaskSet/BodyFootTask.hpp>
#include <Mercury_Controller/ContactSet/SingleContact.hpp>
#include <WBDC/WBDC.hpp>
#include <Mercury/Mercury_Model.hpp>
#include <Mercury/Mercury_Definition.h>
#include <ParamHandler/ParamHandler.hpp>
#include <Planner/PIPM_FootPlacementPlanner/Reversal_LIPM_Planner.hpp>
#include <Utils/DataManager.hpp>

BodyFootPlanningCtrl::BodyFootPlanningCtrl(RobotSystem* robot, int swing_foot, Planner* planner):
    Controller(robot),
    swing_foot_(swing_foot),
    num_planning_(0),
    planning_frequency_(0.),
    replan_moment_(0.),
    ctrl_start_time_(0.)
{
    planner_ = planner;
    curr_foot_pos_des_.setZero();
    curr_foot_vel_des_.setZero();
    curr_foot_acc_des_.setZero();

    body_foot_task_ = new BodyFootTask(robot, swing_foot);
    if(swing_foot == mercury_link::leftFoot) {
        single_contact_ = new SingleContact(robot, mercury_link::rightFoot); }
    else if(swing_foot == mercury_link::rightFoot) {
        single_contact_ = new SingleContact(robot, mercury_link::leftFoot); }
    else printf("[Warnning] swing foot is not foot: %i\n", swing_foot);

    std::vector<bool> act_list;
    act_list.resize(mercury::num_qdot, true);
    for(int i(0); i<mercury::num_virtual; ++i) act_list[i] = false;


    wbdc_ = new WBDC(act_list);

    wbdc_data_ = new WBDC_ExtraData();
    wbdc_data_->tau_min = dynacore::Vector(mercury::num_act_joint);
    wbdc_data_->tau_max = dynacore::Vector(mercury::num_act_joint);

    for(int i(0); i<mercury::num_act_joint; ++i){
        wbdc_data_->tau_max[i] = 100.0;
        wbdc_data_->tau_min[i] = -100.0;
    }

    com_estimator_ = new LIPM_KalmanFilter();
    sp_ = Mercury_StateProvider::getStateProvider();
    // printf("[BodyFoot Controller] Constructed\n");
}

BodyFootPlanningCtrl::~BodyFootPlanningCtrl(){
    delete body_foot_task_;
    delete single_contact_;
    delete wbdc_;
    delete wbdc_data_;
}

void BodyFootPlanningCtrl::OneStep(dynacore::Vector & gamma){
    _PreProcessing_Command();
    state_machine_time_ = sp_->curr_time_ - ctrl_start_time_;

    gamma.setZero();
    _single_contact_setup();
    _task_setup();
    _body_foot_ctrl(gamma);

    _PostProcessing_Command();
}

void BodyFootPlanningCtrl::_body_foot_ctrl(dynacore::Vector & gamma){
    wbdc_->UpdateSetting(A_, Ainv_, coriolis_, grav_);
    wbdc_->MakeTorque(task_list_, contact_list_, gamma, wbdc_data_);

    int offset(0);
    if(swing_foot_ == mercury_link::rightFoot) offset = 3;
    for(int i(0); i<3; ++i)
        sp_->reaction_forces_[i + offset] = wbdc_data_->opt_result_[i];
}

void BodyFootPlanningCtrl::_task_setup(){
    dynacore::Vector pos_des(3 + 4 + 3);
    dynacore::Vector vel_des(body_foot_task_->getDim());
    dynacore::Vector acc_des(body_foot_task_->getDim());
    pos_des.setZero(); vel_des.setZero(); acc_des.setZero();

    // CoM Pos
    pos_des.head(3) = ini_com_pos_;
    if(b_set_height_target_) pos_des[2] = des_com_height_;

    // Orientation
    dynacore::Vect3 rpy_des;
    dynacore::Quaternion quat_des;
    rpy_des.setZero();

    dynacore::convert(rpy_des, quat_des);
    pos_des[3] = quat_des.w();
    pos_des[4] = quat_des.x();
    pos_des[5] = quat_des.y();
    pos_des[6] = quat_des.z();


    // set relaxed op direction
    // cost weight setup
    // bool b_height_relax(false);
    bool b_height_relax(true);
    if(b_height_relax){
        std::vector<bool> relaxed_op(body_foot_task_->getDim(), false);
        relaxed_op[0] = true; // X
        relaxed_op[1] = true; // Y
        relaxed_op[2] = true; // Z
        relaxed_op[3] = true; // Rx
        relaxed_op[4] = true; // Ry
        relaxed_op[5] = true; // Rz

        body_foot_task_->setRelaxedOpCtrl(relaxed_op);

        int prev_size(wbdc_data_->cost_weight.rows());
        wbdc_data_->cost_weight.conservativeResize( prev_size + 6);
        wbdc_data_->cost_weight[prev_size] = 0.0001;
        wbdc_data_->cost_weight[prev_size+1] = 0.0001;
        wbdc_data_->cost_weight[prev_size+2] = 1000.;
        wbdc_data_->cost_weight[prev_size+3] = 10.;
        wbdc_data_->cost_weight[prev_size+4] = 10.;
        wbdc_data_->cost_weight[prev_size+5] = 0.001;
    }

    _CoMEstiamtorUpdate();
    _CheckPlanning();

    double traj_time = state_machine_time_ - replan_moment_;
    // Foot Position Task
    double pos[3];
    double vel[3];
    double acc[3];

    // printf("time: %f\n", state_machine_time_);
    foot_traj_.getCurvePoint(traj_time, pos);
    foot_traj_.getCurveDerPoint(traj_time, 1, vel);
    foot_traj_.getCurveDerPoint(traj_time, 2, acc);
    // printf("pos:%f, %f, %f\n", pos[0], pos[1], pos[2]);

    for(int i(0); i<3; ++i){
        curr_foot_pos_des_[i] = pos[i];
        curr_foot_vel_des_[i] = vel[i];
        curr_foot_acc_des_[i] = acc[i];

        pos_des[i + 7] = curr_foot_pos_des_[i];
        vel_des[i + 6] = curr_foot_vel_des_[i];
        acc_des[i + 6] = curr_foot_acc_des_[i];
    }
    // dynacore::pretty_print(vel_des, std::cout, "[Ctrl] vel des");
    // Push back to task list
    body_foot_task_->UpdateTask(&(pos_des), vel_des, acc_des);
    task_list_.push_back(body_foot_task_);
    // dynacore::pretty_print(wbdc_data_->cost_weight,std::cout, "cost weight");
}

void BodyFootPlanningCtrl::_CheckPlanning(){
    if( state_machine_time_ > (end_time_/(planning_frequency_ + 1.) * (num_planning_ + 1.) + 0.002) ){ // + 0.002 is to account one or two more ticks before the end of phase
        // printf("time (state/end): %f, %f\n", state_machine_time_, end_time_);
        // printf("planning freq: %f\n", planning_frequency_);
        // printf("num_planning: %i\n", num_planning_);
        _Replanning();
        ++num_planning_;
    }
}

void BodyFootPlanningCtrl::_Replanning(){
    dynacore::Vect3 com_pos, com_vel, target_loc;
    // Direct value used
    robot_sys_->getCoMPosition(com_pos);
    robot_sys_->getCoMVelocity(com_vel);

    // Estimated value used
    for(int i(0); i<2; ++i){
        com_pos[i] = sp_->estimated_com_state_[i];
        com_vel[i] = sp_->estimated_com_state_[i + 2];
    }

    OutputReversalPL pl_output;
    ParamReversalPL pl_param;
    pl_param.swing_time = end_time_ - state_machine_time_
        + transition_time_ * transition_phase_ratio_
        + stance_time_ * double_stance_ratio_;

    pl_param.des_loc = sp_->des_location_;
    pl_param.stance_foot_loc = sp_->global_pos_local_;

    if(swing_foot_ == mercury_link::leftFoot)
        pl_param.b_positive_sidestep = true;
    else
        pl_param.b_positive_sidestep = false;

    planner_->getNextFootLocation(com_pos + sp_->global_pos_local_,
            com_vel,
            target_loc,
            &pl_param, &pl_output);
    // Time Modification
    replan_moment_ = state_machine_time_;
    end_time_ += pl_output.time_modification;

    // dynacore::pretty_print(target_loc, std::cout, "planed foot loc");
    // dynacore::pretty_print(sp_->global_pos_local_, std::cout, "global loc");

    target_loc -= sp_->global_pos_local_;

    // dynacore::pretty_print(target_loc, std::cout, "next foot loc");
    _SetBspline(curr_foot_pos_des_, curr_foot_vel_des_, curr_foot_acc_des_, target_loc);
}

void BodyFootPlanningCtrl::_single_contact_setup(){
    single_contact_->UpdateContactSpec();
    contact_list_.push_back(single_contact_);
    wbdc_data_->cost_weight = dynacore::Vector::Zero(single_contact_->getDim());
    for(int i(0); i<single_contact_->getDim(); ++i){
        wbdc_data_->cost_weight[i] = 1.;
    }
    wbdc_data_->cost_weight[2] = 0.0001;
}

void BodyFootPlanningCtrl::FirstVisit(){
    // printf("[Body Foot Ctrl] Start\n");

    robot_sys_->getPos(swing_foot_, ini_foot_pos_);
    ctrl_start_time_ = sp_->curr_time_;
    state_machine_time_ = 0.;
    replan_moment_ = 0.;

    dynacore::Vect3 zero;
    zero.setZero();
    default_target_loc_[0] = sp_->Q_[0];
    //default_target_loc_[2] = ini_foot_pos_[2];
    default_target_loc_[2] = 0.0;
    _SetBspline(ini_foot_pos_, zero, zero, default_target_loc_);

    // _Replanning();
    num_planning_ = 0;

    dynacore::Vect3 com_pos, com_vel;
    robot_sys_->getCoMPosition(com_pos);
    robot_sys_->getCoMVelocity(com_vel);
    dynacore::Vector input_state(4);
    input_state[0] = com_pos[0];   input_state[1] = com_pos[1];
    input_state[2] = com_vel[0];   input_state[3] = com_vel[1];

    com_estimator_->EstimatorInitialization(input_state);
    _CoMEstiamtorUpdate();

    // dynacore::pretty_print(ini_foot_pos_, std::cout, "ini foot pos");
}

void BodyFootPlanningCtrl::_CoMEstiamtorUpdate(){
    dynacore::Vect3 com_pos, com_vel;
    robot_sys_->getCoMPosition(com_pos);
    robot_sys_->getCoMVelocity(com_vel);
    dynacore::Vector input_state(4);
    input_state[0] = com_pos[0];   input_state[1] = com_pos[1];
    input_state[2] = com_vel[0];   input_state[3] = com_vel[1];
    com_estimator_->InputData(input_state);
    com_estimator_->Output(sp_->estimated_com_state_);
}

void BodyFootPlanningCtrl::_SetBspline(const dynacore::Vect3 & st_pos,
        const dynacore::Vect3 & st_vel,
        const dynacore::Vect3 & st_acc,
        const dynacore::Vect3 & target_pos){
    // Trajectory Setup
    double init[9];
    double fin[9];
    double** middle_pt = new double*[1];
    middle_pt[0] = new double[3];

    // printf("time (state/end): %f, %f\n", state_machine_time_, end_time_);
    double portion = (1./end_time_) * (end_time_/2. - state_machine_time_);
    // printf("portion: %f\n\n", portion);
    // Initial and final position & velocity & acceleration
    for(int i(0); i<3; ++i){
        // Initial
        init[i] = st_pos[i];
        init[i+3] = st_vel[i];
        init[i+6] = st_acc[i];
        // Final
        fin[i] = target_pos[i];
        fin[i+3] = 0.;
        fin[i+6] = 0.;

        if(portion > 0.)
            middle_pt[0][i] = (st_pos[i] + target_pos[i])*portion;
        else
            middle_pt[0][i] = (st_pos[i] + target_pos[i])/2.;
    }
    if(portion > 0.)  middle_pt[0][2] = swing_height_;

    foot_traj_.SetParam(init, fin, middle_pt, end_time_ - replan_moment_);

    delete [] *middle_pt;
    delete [] middle_pt;
}


void BodyFootPlanningCtrl::LastVisit(){
}

bool BodyFootPlanningCtrl::EndOfPhase(){
    if(state_machine_time_ > end_time_){
        // printf("[Body Foot Ctrl] End\n");
        return true;
    }
    return false;
}

void BodyFootPlanningCtrl::CtrlInitialization(const std::string & setting_file_name){
    robot_sys_->getCoMPosition(ini_com_pos_);
    std::vector<double> tmp_vec;

    // Setting Parameters
    ParamHandler handler(MercuryConfigPath + setting_file_name + ".yaml");
    handler.getValue("swing_height", swing_height_);

    handler.getVector("default_target_foot_location", tmp_vec);
    for(int i(0); i<3; ++i){
        default_target_loc_[i] = tmp_vec[i];
    }

    // Feedback Gain
    handler.getVector("Kp", tmp_vec);
    for(int i(0); i<tmp_vec.size(); ++i){
        ((BodyFootTask*)body_foot_task_)->Kp_vec_[i] = tmp_vec[i];
    }
    handler.getVector("Kd", tmp_vec);
    for(int i(0); i<tmp_vec.size(); ++i){
        ((BodyFootTask*)body_foot_task_)->Kd_vec_[i] = tmp_vec[i];
    }

    // Torque limit
    handler.getVector("torque_max", tmp_vec);
    for(int i(0); i<mercury::num_act_joint; ++i){
        wbdc_data_->tau_max[i] = tmp_vec[i];
    }

    handler.getVector("torque_min", tmp_vec);
    for(int i(0); i<mercury::num_act_joint; ++i){
        wbdc_data_->tau_min[i] = tmp_vec[i];
    }

    ((Reversal_LIPM_Planner*)planner_)->CheckEigenValues(double_stance_ratio_*stance_time_ + transition_phase_ratio_*transition_time_ + end_time_);
    printf("[Body Foot Planning Ctrl] Parameter Setup Completed\n");
}