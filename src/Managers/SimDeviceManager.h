#ifndef SIMDEVICEMANAGER_H
#define SIMDEVICEMANAGER_H

#include <Device/SimDeviceInfo.h>
#include <Device/SimDevices.h>
#include <ModelGraph/PhyModelGraphAgent.h>
#include <URDFParser/RobotProxyURDFParser.h>

using namespace std;

class SimDeviceManager
{

public:
    PhyModelGraphAgent              m_rPhyMGAgent;
    vector<SimDeviceInfo>           m_SimDevices;

    /// <DeviceName, SimGPS*>
    map<string, SimGPS*>               m_SimGPSList;
    map<string, SimVicon*>             m_SimViconList;
    map<string, SimCam*>               m_SimCamList;
    map<string, SimLaser2D*>           m_SimLaser2DList;
    map<string, SimLaser3D*>           m_SimLaser3DList;
    map<string, SimpleController*>     m_SimpleControllerList;
    map<string, CarController*>        m_CarControllerList;

    /// Constructor
    SimDeviceManager();

    /// Initializers
    bool Init(PhyModelGraphAgent& pPhyMGAgent, GLSceneGraph& rSceneGraph,
              tinyxml2::XMLDocument& doc, string sProxyName);
    void InitDevices(SceneGraph::GLSceneGraph&  rSceneGraph);
    void InitCamDevice(SimDeviceInfo& Device, string sCameraModel,
                       SceneGraph::GLSceneGraph&  rSceneGraph);
    void InitViconDevice(SimDeviceInfo& Device);
    void InitController(SimDeviceInfo& Device);

    /// Update devices
    void UpdateAlLDevice();

    /// Get pointers to all devices
    SimpleController* GetSimpleController(string name);
    SimCam* GetSimCam(string name);
    SimGPS* GetSimGPS(string name);
    SimVicon* GetSimVecon(string name);

};

#endif // SIMDEVICEMANAGER_H