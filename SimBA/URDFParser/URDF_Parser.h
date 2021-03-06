#ifndef URDF_PARSER_H
#define URDF_PARSER_H

#include <vector>
#include <Eigen/Eigen>
#include <URDFParser/TinyXMLTool.h>
#include <ModelGraph/Shape.h>
#include <ModelGraph/Constraint.h>
#include <ModelGraph/SimRaycastVehicle.h>
#include <SimDevices/SimDevices.h>
#include <SimRobots/SimRobot.h>
#include <SimRobots/SimWorld.h>
#include <PbMsgs/BVP.pb.h>
#include <Node/Node.h>

/////////////////////////////////
/// URDF PARSER
/// This file merges the LocalSimURDFParser and the StateKeeperURDFParser
/// There was no reason that there should have been two files, as far as I can
/// tell. They parse the same files, after all.
/// ------------
/// Please notice that the URDF file here is not strictly the URDF format on the
/// internet. Our version is a bit different from the formal variety; We may
/// consider support for the strict URDF format in the future.
/////////////////////////////////

using namespace std;

/// I'm not sure what this does yet, but here it is.
namespace Eigen{
typedef Matrix<double, 6, 1> Vector6d;
}

class URDF_Parser
{
public:
  URDF_Parser();

  // Parses the world for the mesh and conditions.
  bool ParseWorld(XMLDocument& doc, SimWorld& mSimWorld);

  // ParseRobot really parses each of the robot parts, and then generates a set
  // of commands that the PhysicsEngine can use to create bullet objects.
  bool ParseRobot(XMLDocument &doc, SimRobot &m_SimRobot, string sProxyName);

  void ParseShape(string sRobotName, XMLElement *pElement);

  void ParseJoint(string sRobotName, XMLElement *pElement);

  SimRaycastVehicle *ParseRaycastCar(string sRobotName, XMLElement *pElement);

  void ParseSensorShape(string sRobotName, XMLElement *pElement );

  bool ParseCommandLineForPickSensor(string sCommandLine);

  vector<string> GetScemeFromString(string sCommandLine);

  // ParseDevices uses the information given in the Robot.xml file to create the
  // sensor views that we see later in the Sim.
  bool ParseDevices(XMLDocument& doc,
                    SimDevices &m_SimDevices,
                    string sProxyName);

  // This method is used in StateKeeper to initialize the position of every
  // object in the LocalSim.
  bool ParseWorldForInitRobotPose(const char* filename,
                                 vector<Eigen::Vector6d>& vRobotInitPose);

  std::vector<ModelNode*> GetModelNodes(
      std::map<std::string, ModelNode*> mNodes);

  ////////////////////////////////////////

  std::map<std::string, ModelNode*> m_mModelNodes;
  std::map<std::string, ModelNode*> m_mWorldNodes;
  node::node node_;

};

#endif // URDF_PARSER_H
