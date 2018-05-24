#ifndef MOTION_CAPTURE_DATA_MANAGER
#define MOTION_CAPTURE_DATA_MANAGER

#include <Utils/dynacore_pThread.hpp>
#include <Utils/wrap_eigen.hpp>
#include <Filter/filters.hpp>

class Mercury_StateProvider;
class RobotSystem;

////////////////////////////////////////////
// Must to be synchronized with MoCap system
/////////////////////////////////////////////
#define MOCAP_DATA_PORT 51128
#define IP_ADDRESS "192.168.0.144"
#define NUM_MARKERS 13

typedef struct{
  // float condition[NUM_MARKERS];
  double data[NUM_MARKERS * 3];
}message;
////////////////////////////////////////////
typedef struct{
  int visible[NUM_MARKERS];
  double data[NUM_MARKERS*3];
}mercury_message;


class MoCapManager: public dynacore_pThread{
public:
  MoCapManager(RobotSystem* );
  virtual ~MoCapManager(){}

  virtual void run(void);

  dynacore::Quaternion body_quat_;

protected:
  double initialization_duration_;
  dynacore::Vect3 offset_;
  Mercury_StateProvider * sp_;
  dynacore::Matrix R_coord_;

  void _print_message(const mercury_message & msg);
  void _UpdateLEDPosData(const mercury_message & msg);
  void _CoordinateUpdate(mercury_message & msg);
  void _CoordinateChange(mercury_message & msg);
  dynacore::Matrix _GetOrientation(const dynacore::Vect3 &, 
          const dynacore::Vect3 &, const dynacore::Vect3 &);

  int socket_;
  std::vector<dynacore::Vect3> mocap_data_;
  std::vector<int> marker_cond_;

  dynacore::Vector led_pos_data_;
  // dynacore::Vector led_kin_data_;
  dynacore::Vector led_pos_raw_data_;

  int lfoot_idx;
  int rfoot_idx;

  std::vector<filter*> vel_filter_;
  dynacore::Vect3 body_led_vel_;

  const RobotSystem* robot_sys_;
};

#endif
