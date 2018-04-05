#ifndef WHOLE_BODY_DYNAMIC_CONTROL_H
#define WHOLE_BODY_DYNAMIC_CONTROL_H

#include <WBC.hpp>
#include <Utils/utilities.hpp>
#include <Optimizer/Goldfarb/QuadProg++.hh>

#include "WBDC_Task.hpp"
#include "WBDC_ContactSpec.hpp"

class WBDC_ExtraData{
 public:
  dynacore::Vector cost_weight;
  dynacore::Vector tau_min;
  dynacore::Vector tau_max;
  dynacore::Vector opt_result_;

  WBDC_ExtraData(){}
  ~WBDC_ExtraData(){}
};

class WBDC: public WBC{
 public:
  WBDC(const std::vector<bool> & act_list);
  virtual ~WBDC(){}

  virtual void UpdateSetting(const dynacore::Matrix & A,
                             const dynacore::Matrix & Ainv,
                             const dynacore::Vector & cori,
                             const dynacore::Vector & grav,
                             void* extra_setting = NULL);

  virtual void MakeTorque(const std::vector<Task*> & task_list,
                          const std::vector<ContactSpec*> & contact_list,
                          dynacore::Vector & cmd,
                          void* extra_input = NULL);

private:
  void _DimesionSetting(const std::vector<Task*> & task_list,
                        const std::vector<ContactSpec*> & contact_list);
  void _MatrixInitialization();
  void _SetEqualityConstraint();
  void _SetInEqualityConstraint();

  void _ContactBuilding(const std::vector<ContactSpec*> & contact_list);
  void _TaskHierarchyBuilding(const std::vector<Task*> & task_list);

  void _GetSolution(dynacore::Vector & cmd);

  int dim_opt_;
  int dim_eq_cstr_; // equality constraints
  int dim_ieq_cstr_; // inequality constraints
  WBDC_ExtraData* data_;

  GolDIdnani::GVect<double> z;
  // Cost
  GolDIdnani::GMatr<double> G;
  GolDIdnani::GVect<double> g0;

  // Equality
  GolDIdnani::GMatr<double> CE;
  GolDIdnani::GVect<double> ce0;

  // Inequality
  GolDIdnani::GMatr<double> CI;
  GolDIdnani::GVect<double> ci0;

  int dim_rf_;
  int dim_relaxed_task_;
  int dim_cam_;
  int dim_rf_cstr_;

  dynacore::Matrix tot_tau_Mtx_;
  dynacore::Vector tot_tau_Vect_;

  dynacore::Matrix S_delta_;
  dynacore::Matrix Uf_;
  dynacore::Vector uf_ieq_vec_;

  dynacore::Matrix Jc_;
  dynacore::Vector JcDotQdot_;

  dynacore::Matrix B_;
  dynacore::Vector c_;
  dynacore::Vector task_cmd_;

  void _PrintDebug(double i) {
    printf("[WBDC] %f \n", i);
  }

};

#endif