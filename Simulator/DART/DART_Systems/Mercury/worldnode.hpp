#ifndef KRWORLDNODE_H
#define KRWORLDNODE_H

#include <dart/dart.hpp>
#include <dart/gui/osg/osg.hpp>
#include <dart/utils/urdf/urdf.hpp>
#include <osgShadow/LightSpacePerspectiveShadowMap>
#include "Controller.hpp"
#include <Mercury_Controller/Mercury_interface.hpp>

class WorldNode : public dart::gui::osg::WorldNode
{
private:

public:
    WorldNode(const dart::simulation::WorldPtr & world, osgShadow::MinimalShadowMap *);
    virtual ~WorldNode();

    void customPreStep() override;
    void setTorqueCommand(Eigen::VectorXd& gamma);
    void holdpelvis();
    void holdhorizontal();
    void Jpos_control();

    bool pelvis_hold;

    interface* interface_;
    Mercury_SensorData* sensor_data_;
    std::vector<double> cmd_;

protected:
    int mDof;
     Eigen::VectorXd mQ;
    Eigen::VectorXd mQdot;
    Eigen::VectorXd mTorqueCommand;
    
    void _DART_JPosCtrl();
    void _WBDC_Ctrl();
	dart::dynamics::SkeletonPtr robot_;
	dart::dynamics::SkeletonPtr robot_ctr_;
    dart::dynamics::BodyNode* Pelvis_;

    Eigen::Vector3d Pelvis_pos_init_;

	std::unique_ptr<Controller> mController;
		
};

#endif /* KRWORLDNODE_H */