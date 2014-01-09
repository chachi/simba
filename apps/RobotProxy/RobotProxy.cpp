/*
   RobotProxy, By luma. 2013.03
   Edited by bminortx.
 */

#include <RobotProxy.h>

using namespace std;
using namespace CVarUtils;

////////////////////////////////////////////////////////////////////////
/// CONSTRUCTOR
/// RobotProxy does not get produced willy-nilly; it is only produced if there
/// is a SIM.
////////////////////////////////////////////////////////////////////////

RobotProxy::RobotProxy(
    SceneGraph::GLSceneGraph& glGraph,  //< Input: reference to glGraph
    const std::string& sProxyName,      //< Input: name of robot proxy
    const std::string& sRobotURDF,      //< Input: location of meshes, maps etc
    const std::string& sWorldURDF,
    const std::string& sServerName,
    const std::string& sPoseFileName
    )
  : m_rSceneGraph( glGraph ), m_sProxyName(sProxyName),
    m_sWorldURDFFile(sWorldURDF), m_sRobotURDFFile(sRobotURDF){

  // This is used to interface with the user's main function.
  m_Node.init(sProxyName);

  // 1. Parse world.xml file.
  if( ParseWorld(m_sWorldURDFFile.c_str(), m_WorldManager) != true){
    cout<<"[RobotProxy] Cannot parse "<< m_sWorldURDFFile<<
          ". Please check if the file exist and the syntax is valid."<<endl;
    exit(-1);
  }

  // 2. Read Robot.xml file.
  XMLDocument RobotURDF;
  if(GetXMLdoc(sRobotURDF, RobotURDF)!= true){
    cout<<"[RobotProxy] Cannot open "<<sRobotURDF<<
          ". Please check if the file exist and the syntax is valid."<<endl;
    exit(-1);
  }

  // 3. Initialize agent between Physics Engine and ModelGraph
  if(m_PhyMGAgent.init() != true){
    exit(-1);
  }

  // 4. Init User's Robot and add it to RobotManager
  m_RobotManager.Init(m_sProxyName, sServerName ,m_PhyMGAgent,m_Render);
  if( m_RobotManager.AddRobot(RobotURDF, m_sProxyName) !=true )
  {
    cout<<"[RobotProxy] Cannot add new robot to RobotManager."<<endl;
    exit(-1);
  }


  ////What to do here....
  ////
  ///////--> get pointer of main robot. (*** temporty code.
  ///////need to be delete once we implement node controller ***)
  m_pMainRobot = m_RobotManager.GetMainRobot();
  //////
  //////

  // 5. Initialize the Network
  if(m_NetworkManager.initNetwork(m_sProxyName, &m_SimDeviceManager,
                                  &m_RobotManager,  sServerName)!=true){
    cout<<"[RobotProxy] You chose to connect to '"<<sServerName<<
          "' but we cannot initialize Network."<<
          "Please make sure the StateKeeper is running."<<endl;
    exit(-1);
  }

  // 6. Initialize the Sim Device (SimCam, SimGPS, SimVicon, etc...)
  if(m_SimDeviceManager.Init(m_PhyMGAgent,  m_rSceneGraph, RobotURDF,
                             m_sProxyName)!= true){
    cout<<"[RobotProxy] Cannot init SimDeviceManager."<<endl;
    exit(-1);
  }

  m_SimpPoseController.Init(sPoseFileName);

  // 7, if run in with network mode, proxy network will publish sim device
  if(sServerName !="WithoutNetwork"){
    if(m_NetworkManager.initDevices()!=true){
      cout<<"[RobotProxy] Cannot init Nextwrok"<<endl;
      exit(-1);
    }
  }
  else{
    cout<<"[RobotProxy] Init Robot Proxy without Network."<<endl;
  }

  cout<<"[RobotProxy] Init RobotProxy Success!"<<endl;

}

////////////////////////////////////////////////////////////////////////
/// FUNCTIONS
////////////////////////////////////////////////////////////////////////

// InitReset will populate SceneGraph with objects, and
// register these objects with the simulator.

void RobotProxy::InitReset()
{
  m_rSceneGraph.Clear();

  m_light.SetPosition( m_WorldManager.vLightPose[0],
                       m_WorldManager.vLightPose[1],
                       m_WorldManager.vLightPose[2]);
  m_rSceneGraph.AddChild( &m_light );

  // init world without mesh
  if (m_WorldManager.m_sMesh =="NONE")
  {
    cout<<"[RobotRroxy] Try init empty world."<<endl;
    m_grid.SetNumLines(20);
    m_grid.SetLineSpacing(1);
    m_rSceneGraph.AddChild(&m_grid);

    double dThickness = 1;
    m_ground.SetPose( 0,0, dThickness/2.0,0,0,0 );
    m_ground.SetExtent( 200,200, dThickness );
    m_rSceneGraph.AddChild( &m_ground );

    BoxShape bs = BoxShape(100, 100, 1/2.0f);
    Body* ground = new Body("Ground", bs);
    ground->m_dMass = 0;
    ground->SetWPose( m_ground.GetPose4x4_po() );
    m_PhyMGAgent.m_Agent.GetPhys()->RegisterObject(ground, "Ground",
                                                   m_ground.GetPose());
    m_PhyMGAgent.m_Agent.SetFriction("Ground", 888);
  }
  // init world with mesh
  // maybe dangerous to always reload meshes?
  /// maybe we should separate Init from Reset?
  else {
    try
    {
      cout<<"[RobotRroxy] Try init word with mesh: "
         <<m_WorldManager.m_sMesh<<endl;
      m_Map.Init(m_WorldManager.m_sMesh);
      m_Map.SetPerceptable(true);
      m_Map.SetScale(m_WorldManager.iScale);
      m_Map.SetPosition(m_WorldManager.vWorldPose[0],
                        m_WorldManager.vWorldPose[1],
                        m_WorldManager.vWorldPose[2]);
      m_rSceneGraph.AddChild( &m_Map );
    } catch (std::exception e) {
      printf( "Cannot load world map\n");
      exit(-1);
    }
  }
  m_Render.AddToScene( &m_rSceneGraph );
}

//////////////////////////////////////////////////////////////////

// Apply the camera's pose directly to the SimCamera

void RobotProxy::ApplyCameraPose(Eigen::Vector6d dPose){
  Eigen::Vector6d InvalidPose;
  InvalidPose<<1,88,99,111,00,44;
  if(dPose == InvalidPose){
  }
  else{
    string sMainRobotName = m_pMainRobot->GetRobotName();

    // update RGB camera pose
    string sNameRGBCam   = "RGBLCamera@" + sMainRobotName;
    m_SimDeviceManager.GetSimCam(sNameRGBCam)->UpdateByPose(_Cart2T(dPose));

    // update Depth camera pose
    string sNameDepthCam = "DepthLCamera@"+sMainRobotName;
    m_SimDeviceManager.GetSimCam(sNameDepthCam)->UpdateByPose(_Cart2T(dPose));
  }
}

void RobotProxy::ApplyPoseToEntity(string sName, Eigen::Vector6d dPose){
  m_PhyMGAgent.m_Agent.SetEntity6Pose(sName, dPose);
}

// ---- Step Forward
void RobotProxy::StepForward(){
  m_PhyMGAgent.m_Agent.GetPhys()->StepSimulation();
  m_Render.UpdateScene();
}

//////////////////////////////////////////////////////////////////

// Scan all SimDevices and send the simulated camera images to Pangolin.
// Right now, we can only support up to two windows.

bool RobotProxy::SetImagesToWindow(SceneGraph::ImageView& LSimCamWnd,
                                   SceneGraph::ImageView& RSimCamWnd ){
  int WndCounter = 0;

  for(unsigned int i =0 ; i!= m_SimDeviceManager.m_SimDevices.size(); i++){
    SimDeviceInfo Device = m_SimDeviceManager.m_SimDevices[i];

    for(unsigned int j=0;j!=Device.m_vSensorList.size();j++){

      string sSimCamName = Device.m_vSensorList[j];
      SimCam* pSimCam = m_SimDeviceManager.GetSimCam(sSimCamName);

      SceneGraph::ImageView* ImageWnd;

      // get pointer to window
      if(WndCounter == 0){
        ImageWnd = &LSimCamWnd;
      }
      else if(WndCounter == 1){
        ImageWnd = &RSimCamWnd;
      }

      WndCounter++;

      // set image to window
      if (pSimCam->m_iCamType == 5){       // for depth image
        float* pImgbuf = (float*) malloc( pSimCam->g_nImgWidth *
                                          pSimCam->g_nImgHeight *
                                          sizeof(float) );

        if(pSimCam->capture(pImgbuf)==true){
          ImageWnd->SetImage(pImgbuf, pSimCam->g_nImgWidth,
                             pSimCam->g_nImgHeight,
                             GL_INTENSITY, GL_LUMINANCE, GL_FLOAT);
          free(pImgbuf);
        }
        else{
          cout<<"[SetImagesToWindow] Set depth Image fail"<<endl;
          return false;
        }
      }
      else if(pSimCam->m_iCamType == 2){   // for RGB image
        char* pImgbuf= (char*)malloc (pSimCam->g_nImgWidth *
                                      pSimCam->g_nImgHeight * 3);

        if(pSimCam->capture(pImgbuf)==true){
          ImageWnd->SetImage(pImgbuf, pSimCam->g_nImgWidth,
                             pSimCam->g_nImgHeight,
                             GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
          free(pImgbuf);
        }
        else{
          cout<<"[SetImagesToWindow] Set RGB Image fail"<<endl;
          return false;
        }
      }
      else if(pSimCam->m_iCamType == 1){    //to show greyscale image
        char* pImgbuf= (char*)malloc (pSimCam->g_nImgWidth *
                                      pSimCam->g_nImgHeight);
        if(pSimCam->capture(pImgbuf)==true){
          ImageWnd->SetImage(pImgbuf, pSimCam->g_nImgWidth,
                             pSimCam->g_nImgHeight,
                             GL_INTENSITY, GL_LUMINANCE, GL_UNSIGNED_BYTE);
          free(pImgbuf);
        }
        else{
          cout<<"[SetImagesToWindow] Set Gray Image fail"<<endl;
          return false;
        }
      }
    }
  }
  return true;
}

////////////////////
/// INPUT KEYS
////////////////////

void RobotProxy::LeftKey(){
  string sMainRobotName = m_pMainRobot->GetRobotName();

  // update RGB camera pose
  string sNameRGBCam   = "RGBLCamera@" + sMainRobotName;

  Eigen::Vector6d dPoseRGB =
      _T2Cart(m_SimDeviceManager.GetSimCam(sNameRGBCam)->GetCameraPose() );
  dPoseRGB(5,0) = dPoseRGB(5,0) - 0.1;
  m_SimDeviceManager.GetSimCam(sNameRGBCam)->UpdateByPose(_Cart2T(dPoseRGB));

  // update Depth camera pose
  string sNameDepthCam = "DepthLCamera@"+sMainRobotName;

  Eigen::Vector6d dPoseDepth =
      _T2Cart(m_SimDeviceManager.GetSimCam(sNameDepthCam)->GetCameraPose() );
  dPoseDepth(5,0) = dPoseDepth(5,0) - 0.1;
  m_SimDeviceManager.
      GetSimCam(sNameDepthCam)->UpdateByPose(_Cart2T(dPoseDepth));

  //            string sMainRobotName = m_pMainRobot->GetRobotName();
  //            string sName = "RCamera@" + sMainRobotName;
  //            Eigen::Vector6d dPose;
  //            m_PhyMGAgent.m_Agent.GetEntity6Pose(sName, dPose);
  //            dPose(0,0) = dPose(0,0) + 1;
  //            m_PhyMGAgent.m_Agent.SetEntity6Pose(sName, dPose);
}

void RobotProxy::RightKey(){
  string sMainRobotName = m_pMainRobot->GetRobotName();

  // update RGB camera pose
  string sNameRGBCam   = "RGBLCamera@" + sMainRobotName;

  Eigen::Vector6d dPoseRGB =
      _T2Cart(m_SimDeviceManager.GetSimCam(sNameRGBCam)->GetCameraPose() );
  dPoseRGB(5,0) = dPoseRGB(5,0) + 0.1;
  m_SimDeviceManager.GetSimCam(sNameRGBCam)->UpdateByPose(_Cart2T(dPoseRGB));

  // update Depth camera pose
  string sNameDepthCam = "DepthLCamera@"+sMainRobotName;

  Eigen::Vector6d dPoseDepth =
      _T2Cart(m_SimDeviceManager.GetSimCam(sNameDepthCam)->GetCameraPose() );
  dPoseDepth(5,0) = dPoseDepth(5,0) + 0.1;
  m_SimDeviceManager.
      GetSimCam(sNameDepthCam)->UpdateByPose(_Cart2T(dPoseDepth));
}

void RobotProxy::ForwardKey(){
  //  you should update the pose of the rig and then the poses of the cameras would always be relative to the rig
  string sMainRobotName = m_pMainRobot->GetRobotName();

  // update RGB camera pose
  string sNameRGBCam   = "RGBLCamera@" + sMainRobotName;

  Eigen::Vector6d dPoseRGB =
      _T2Cart( m_SimDeviceManager.GetSimCam(sNameRGBCam)->GetCameraPose() );
  dPoseRGB(1,0) = dPoseRGB(1,0) + 1;
  m_SimDeviceManager.GetSimCam(sNameRGBCam)->UpdateByPose(_Cart2T(dPoseRGB));

  // update Depth camera pose
  string sNameDepthCam = "DepthLCamera@"+sMainRobotName;

  Eigen::Vector6d dPoseDepth =
      _T2Cart(m_SimDeviceManager.GetSimCam(sNameDepthCam)->GetCameraPose() );
  dPoseDepth(1,0) = dPoseDepth(1,0) + 1;
  m_SimDeviceManager.GetSimCam(sNameDepthCam)->UpdateByPose(_Cart2T(dPoseDepth));
}

void RobotProxy::ReverseKey(){
  string sMainRobotName = m_pMainRobot->GetRobotName();

  // update RGB camera pose
  string sNameRGBCam   = "RGBLCamera@" + sMainRobotName;

  Eigen::Vector6d dPoseRGB =
      _T2Cart(m_SimDeviceManager.GetSimCam(sNameRGBCam)->GetCameraPose() );
  dPoseRGB(1,0) = dPoseRGB(1,0) - 1;
  m_SimDeviceManager.GetSimCam(sNameRGBCam)->UpdateByPose(_Cart2T(dPoseRGB));

  // update Depth camera pose
  string sNameDepthCam = "DepthLCamera@"+sMainRobotName;

  Eigen::Vector6d dPoseDepth =
      _T2Cart(m_SimDeviceManager.GetSimCam(sNameDepthCam)->GetCameraPose() );
  dPoseDepth(1,0) = dPoseDepth(1,0) - 1;
  m_SimDeviceManager.
      GetSimCam(sNameDepthCam)->UpdateByPose(_Cart2T(dPoseDepth));
}


///////////////////////////////////////////////////////////////////
/////
///// MAIN LOOP:
///// 1. Parses arguments in xml file
///// 2. Develops the SceneGraph
///// 3. Renders the objects in the Sim.
/////
///////////////////////////////////////////////////////////////////


int main( int argc, char** argv )
{
  // parse command line arguments
  if(argc){
    std::cout<<"USAGE: Robot -n <ProxyName> -r <robot.xml directory>"<<
               "-w <world.xml directory> -s <StateKeeper Option>"<<std::endl;
    std::cout<<"Options:"<<std::endl;
    std::cout<<"--ProxyName, -n         ||   Name of this RobotProxy."
            <<std::endl;
    std::cout<<"--Robot.xml, -r         ||   robot.xml's directory"
            <<std::endl;
    std::cout<<"--World.xml, -w         ||   world.xml's directory."
            <<std::endl;
    std::cout<<"--Statekeeper Option -s ||   input 'StateKeeperName',"<<
               " 'WithoutStateKeeper', or 'WithoutNetwork'"<<std::endl;

  }
  GetPot cl( argc, argv );
  std::string sProxyName = cl.follow( "SimWorld", "-n" );
  std::string sRobotURDF = cl.follow("", "-r");
  std::string sWorldURDF = cl.follow( "", "-w" );
  std::string sPoseFile  = cl.follow("None",1,"-p");
  std::string sServerOption = cl.follow("WithoutStateKeeper", "-s");

  //Start our SceneGraph interface
  pangolin::CreateGlutWindowAndBind(sProxyName,640,480);
  SceneGraph::GLSceneGraph::ApplyPreferredGlSettings();
  glClearColor(0, 0, 0, 1);
  glewInit();
  SceneGraph::GLSceneGraph  glGraph;

  // Initialize a RobotProxy.
  RobotProxy mProxy( glGraph, sProxyName, sRobotURDF,
                     sWorldURDF, sServerOption, sPoseFile);
  mProxy.InitReset();


  //---------------------------------------------------------------------------
  // <Pangolin boilerplate>
  const SceneGraph::AxisAlignedBoundingBox bbox =
      glGraph.ObjectAndChildrenBounds();
  const Eigen::Vector3d center = bbox.Center();
  const double size = bbox.Size().norm();
  const double far = 2*size;
  const double near = far / 1E3;

  // Define Camera Render Object (for view / scene browsing)
  pangolin::OpenGlRenderState stacks3d(
        pangolin::ProjectionMatrix(640,480,420,420,320,240,near,far),
        pangolin::ModelViewLookAt(center(0), center(1) + size,
                                  center(2) - size/4,
                                  center(0), center(1), center(2),
                                  pangolin::AxisNegZ) );

  // We define a new view which will reside within the container.
  pangolin::View view3d;

  // We set the views location on screen and add a handler which will
  // let user input update the model_view matrix (stacks3d) and feed through
  // to our scenegraph
  view3d.SetBounds( 0.0, 1.0, 0.0, 0.75/*, -640.0f/480.0f*/ );
  view3d.SetHandler( new SceneGraph::HandlerSceneGraph( glGraph, stacks3d) );
  view3d.SetDrawFunction( SceneGraph::ActivateDrawFunctor( glGraph, stacks3d) );

  // window for display image capture from simcam
  SceneGraph::ImageView LSimCamImage(true,true);
  LSimCamImage.SetBounds( 0.0, 0.5, 0.5, 1.0/*, 512.0f/384.0f*/ );

  // window for display image capture from simcam
  SceneGraph::ImageView RSimCamImage(true,true);
  RSimCamImage.SetBounds( 0.5, 1.0, 0.5, 1.0/*, 512.0f/384.0f */);


  // Add our views as children to the base container.
  pangolin::DisplayBase().AddDisplay( view3d );
  pangolin::DisplayBase().AddDisplay( LSimCamImage );
  pangolin::DisplayBase().AddDisplay( RSimCamImage );

  //----------------------------------------------------------------------------
  // Register a keyboard hook to trigger the reset method
  RegisterKeyPressCallback( pangolin::PANGO_CTRL + 'r',
                            boost::bind( &RobotProxy::InitReset, &mProxy ) );

  // Simple asdw control
  RegisterKeyPressCallback( 'a', boost::bind( &RobotProxy::LeftKey, &mProxy ) );
  RegisterKeyPressCallback( 'A', boost::bind( &RobotProxy::LeftKey, &mProxy ) );

  RegisterKeyPressCallback( 's', boost::bind( &RobotProxy::ReverseKey, &mProxy ) );
  RegisterKeyPressCallback( 'S', boost::bind( &RobotProxy::ReverseKey, &mProxy ) );

  RegisterKeyPressCallback( 'd', boost::bind( &RobotProxy::RightKey, &mProxy ) );
  RegisterKeyPressCallback( 'D', boost::bind( &RobotProxy::RightKey, &mProxy ) );

  RegisterKeyPressCallback( 'w', boost::bind( &RobotProxy::ForwardKey, &mProxy ) );
  RegisterKeyPressCallback( 'W', boost::bind( &RobotProxy::ForwardKey, &mProxy ) );

  RegisterKeyPressCallback( ' ', boost::bind( &RobotProxy::StepForward, &mProxy ) );

  //----------------------------------------------------------------------------
  // wait for robot to connect
  //    if(sServerOption== "WithoutStateKeeper")
  //    {
  //        while(mProxy.m_NetworkManager.m_SubscribeNum == 0)
  //        {
  //            cout<<"["<<sProxyName<<"] wait for RPG Device to register"<<endl;
  //            sleep(1);
  //        }
  //    }

  // Default hooks for exiting (Esc) and fullscreen (tab).
  while( !pangolin::ShouldQuit() )
  {
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    view3d.Activate(stacks3d);

    // 1. Read the Camera pose for the update
    if (!sProxyName.empty()) {
      mProxy.ApplyCameraPose(mProxy.m_SimpPoseController.ReadNextPose());
    }

    // 2. Update physics and scene
    mProxy.m_PhyMGAgent.m_Agent.GetPhys()->DebugDrawWorld();
    mProxy.m_PhyMGAgent.m_Agent.GetPhys()->StepSimulation();
    mProxy.m_Render.UpdateScene();

    // 3. Update SimDevices
    mProxy.m_SimDeviceManager.UpdateAlLDevice();

    // 4. Update the Network
    mProxy.m_NetworkManager.UpdateNetWork();

    // 5. Show the image in the current window
    mProxy.SetImagesToWindow(LSimCamImage,RSimCamImage);

    // 6. Refresh screen
    pangolin::FinishGlutFrame();
    usleep( 1E6 / 60 );
  }

  return 0;

}

