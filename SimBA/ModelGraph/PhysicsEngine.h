#ifndef _PHYSICS_ENGINE_H
#define _PHYSICS_ENGINE_H

#include "PhysicsEngineHelpers.h"

//All of our Bullet Objects
//bullet_shape holds the header files Shapes.h and RaycastVehicle.h
#include <ModelGraph/Bullet_shapes/bullet_shape.h>
#include <ModelGraph/Bullet_shapes/bullet_cube.h>
#include <ModelGraph/Bullet_shapes/bullet_cylinder.h>
#include <ModelGraph/Bullet_shapes/bullet_sphere.h>
#include <ModelGraph/Bullet_shapes/bullet_vehicle.h>
#include <ModelGraph/Bullet_shapes/bullet_plane.h>
#include <ModelGraph/Bullet_shapes/bullet_heightmap.h>
#include <ModelGraph/Bullet_shapes/bullet_mesh.h>

//////////////////////////////////////////////////////////
///
/// PhysicsEngine class
/// PhysicsEngine encapsulates all of the Physics engine (Bullet) into one
/// class. It initializes the physics environment, and allows for the addition
/// and deletion of objects. It must also be called to run the physics sim.
///
//////////////////////////////////////////////////////////


class PhysicsEngine
{
public:

  /// CONSTRUCTOR
  PhysicsEngine();

  /// Initializer
  bool Init(double dGravity = -9.8, double dTimeStep = 1.0/30.0,
            double nMaxSubSteps = 10);

  /// ADDING OBJECTS TO THE PHYSICS ENGINE
  /// RegisterObject adds shapes, constraints, and vehicles.
  void RegisterObject(ModelNode *pItem);
  void RegisterDevice(SimDeviceInfo* pDevice);
  bool isVehicle(string Shape);

  /// RUNNING THE SIMULATION
  void DebugDrawWorld();
  void RunDevices();
  void StepSimulation();

  /// PRINT AND DRAW FUNCTIONS
  void PrintAllShapes();

  /// GETTERS - Used for sensors and controllers
  btHinge2Constraint* getHinge2Constraint(string name);
  btHingeConstraint* getHingeConstraint(string name);
  /// RAYCAST VEHICLE METHODS
  Eigen::Vector6d SwitchYaw(Eigen::Vector6d bad_yaw);
  Eigen::Vector6d SwitchWheelYaw(Eigen::Vector6d bad_yaw);
  std::vector<Eigen::Matrix4d> GetVehiclePoses( Vehicle_Entity* Vehicle );
  std::vector<Eigen::Matrix4d> GetVehicleTransform(std::string sVehicleName);
  bool RayCast(const Eigen::Vector3d& dSource,
               const Eigen::Vector3d& dRayVector,
               Eigen::Vector3d& dIntersect, const bool& biDirectional);
  bool RayCastNormal(const Eigen::Vector3d& dSource,
                     const Eigen::Vector3d& dRayVector,
                     Eigen::Vector3d& dNormal);


  /// PUBLIC MEMBER VARIABLES
  GLDebugDrawer	                                          m_DebugDrawer;
  std::map<string, std::shared_ptr<Vehicle_Entity> >    m_mRayVehicles;
  std::map<string, std::shared_ptr<Entity> >            m_mShapes;
  std::map<string, std::shared_ptr<Compound_Entity> >   m_mCompounds;
  std::map<string, btHingeConstraint*>                    m_mHinge;
  std::map<string, btHinge2Constraint*>                   m_mHinge2;
  std::map<string, btGeneric6DofConstraint*>              m_mSixDOF;
  std::map<string, btPoint2PointConstraint*>              m_mPtoP;
  vector<SimDeviceInfo*>                                  m_mDevices;
  std::shared_ptr<btDiscreteDynamicsWorld>              dynamics_world_;
  btVehicleRaycaster* vehicle_raycaster_;

private:

  /// PRIVATE MEMBER VARIABLES
  btDefaultCollisionConfiguration                        m_CollisionConfiguration;
  std::shared_ptr<btCollisionDispatcher>               m_pDispatcher;
  std::shared_ptr<btDbvtBroadphase>                    m_pBroadphase;
  std::shared_ptr<btSequentialImpulseConstraintSolver> m_pSolver;
  double                                                 m_dTimeStep;
  double                                                 m_dGravity;
  int                                                    m_nMaxSubSteps;

};

#endif //PHYSICSENGINE_H
