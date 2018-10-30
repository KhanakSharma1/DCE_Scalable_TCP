#include "ns3/network-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/dce-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/netanim-module.h"
#include "ns3/constant-position-mobility-model.h"
#include "ccnx/misc-tools.h"
#include "ns3/ipv4-global-routing-helper.h" //??????????Find out the use of this helper class???????????

using namespace ns3;
NS_LOG_COMPONENT_DEFINE ("DceIperf");
// ===========================================================================
//
//         node 0                 node 1
//   +----------------+    +----------------+
//   |                |    |                |
//   +----------------+    +----------------+
//   |    10.1.1.1    |    |    10.1.1.2    |
//   +----------------+    +----------------+
//   | point-to-point |    | point-to-point |
//   +----------------+    +----------------+
//           |                     |
//           +---------------------+
//                5 Mbps, 2 ms
//
// 2 nodes : iperf client en iperf server ....
//
// Note : Tested with iperf 2.0.5, you need to modify iperf source in order to
//        allow DCE to have a chance to end an endless loop in iperf as follow:
//        in source named Thread.c at line 412 in method named thread_rest
//        you must add a sleep (1); to break the infinite loop....
// ===========================================================================
static void GetSSStats (Ptr<Node> node, Time at, std::string stack)
  {
    if(stack == "linux")
    {
      DceApplicationHelper process;
      ApplicationContainer apps;
      process.SetBinary ("ss");
      process.SetStackSize (1 << 20);
      process.AddArgument ("-a");
      process.AddArgument ("-e");
      process.AddArgument ("-i");
      apps.Add(process.Install (node));
      apps.Start(at);
    }
  }

int main (int argc, char *argv[])
{
  std::string stack = "ns3";
  bool useUdp = 0;
  std::string bandWidth = "1m";
  std::string tcp = "scalable";
  CommandLine cmd;
  cmd.AddValue ("stack", "Name of IP stack: ns3/linux/freebsd.", stack);
  cmd.AddValue ("udp", "Use UDP. Default false (0)", useUdp);
  cmd.AddValue ("bw", "BandWidth. Default 1m.", bandWidth);

  cmd.AddValue ("tcp", "", tcp);

  cmd.Parse (argc, argv);

  NodeContainer nodes;
  nodes.Create (3);

  PointToPointHelper pointToPoint1, pointToPoint2;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("1ms"));
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("1ms"));
  
  NetDeviceContainer devices1,devices2;
  devices1 = pointToPoint1.Install (nodes.Get(0), nodes.Get(1));
  devices2 = pointToPoint2.Install (nodes.Get(1), nodes.Get(2));

  DceManagerHelper dceManager;
  dceManager.SetTaskManagerAttribute ("FiberManagerType", StringValue ("UcontextFiberManager"));   

  if (stack == "ns3")
    {
      //std::cout << "Inside ns3" << std::endl;
      InternetStackHelper stack;
      stack.Install (nodes);
      dceManager.Install (nodes);
      TypeId tcpTid;
      NS_ABORT_MSG_UNLESS (TypeId::LookupByNameFailSafe (tcp, &tcpTid), "TypeId " << tcp << " not found");
      Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TypeId::LookupByName (tcp)));
    }
  else if (stack == "linux")
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::LinuxSocketFdFactory", "Library", StringValue ("liblinux.so"));
      dceManager.Install (nodes);
      LinuxStackHelper stack;
      stack.Install (nodes);
      stack.SysctlSet (nodes.Get(0),".net.ipv4.tcp_allowed_congestion_control", tcp);
      stack.SysctlSet (nodes.Get(0),".net.ipv4.tcp_congestion_control",         tcp);
      stack.SysctlSet (nodes.Get(1),".net.ipv4.tcp_allowed_congestion_control", tcp);
      stack.SysctlSet (nodes.Get(1),".net.ipv4.tcp_congestion_control",         tcp);
#else
      NS_LOG_ERROR ("Linux kernel stack for DCE is not available. build with dce-linux module.");
      // silently exit
      return 0;
#endif
    }
  else if (stack == "freebsd")
    {
#ifdef KERNEL_STACK
      dceManager.SetNetworkStack ("ns3::FreeBSDSocketFdFactory", "Library", StringValue ("libfreebsd.so"));
      dceManager.Install (nodes);
      FreeBSDStackHelper stack;
      stack.Install (nodes);
#else
      NS_LOG_ERROR ("FreeBSD kernel stack for DCE is not available. build with dce-freebsd module.");
      // silently exit
      return 0;
#endif
    }

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces1 = address.Assign (devices1);

  address.SetBase ("10.1.2.0", "255.255.255.252");
  Ipv4InterfaceContainer interfaces2 = address.Assign (devices2);

  // setup ip routes
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
#ifdef KERNEL_STACK
  if (stack == "linux")
    {
      LinuxStackHelper::PopulateRoutingTables ();
    }
#endif


  DceApplicationHelper dce;
  ApplicationContainer apps;
  std::ostringstream serverIp;

  dce.SetStackSize (1 << 20);

  // Launch iperf client on node 0
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");

  // Extract server IP address
  /*Ptr<Ipv4> ipv4Server = nodes.Get (1)->GetObject<Ipv4> ();
  Ipv4Address serverAddress = ipv4Server->GetAddress (1, 0).GetLocal ();
  serverAddress.Print (serverIp);*/

  dce.AddArgument ("10.1.2.2");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("10");
  if (useUdp)
    {
      dce.AddArgument ("-u");
      dce.AddArgument ("-b");
      dce.AddArgument (bandWidth);
    }

  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.7));
  apps.Stop (Seconds (20));

/*
// Launch iperf client on node 1
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-c");
*/
  // Extract server IP address
  /*Ptr<Ipv4> ipv4Server = nodes.Get (1)->GetObject<Ipv4> ();
  Ipv4Address serverAddress = ipv4Server->GetAddress (1, 0).GetLocal ();
  serverAddress.Print (serverIp);*/

/*  dce.AddArgument ("10.1.1.2");
  dce.AddArgument ("-i");
  dce.AddArgument ("1");
  dce.AddArgument ("--time");
  dce.AddArgument ("10");
  if (useUdp)
    {
      dce.AddArgument ("-u");
      dce.AddArgument ("-b");
      dce.AddArgument (bandWidth);
    }
*/
/*  apps = dce.Install (nodes.Get (0));
  apps.Start (Seconds (0.7));
  apps.Stop (Seconds (20));
*/

  // Launch iperf server on node 2
  dce.SetBinary ("iperf");
  dce.ResetArguments ();
  dce.ResetEnvironment ();
  dce.AddArgument ("-s");
  dce.AddArgument ("-P");
  dce.AddArgument ("1");
  if (useUdp)
    {
      dce.AddArgument ("-u");
    }

  apps = dce.Install (nodes.Get (2));

  pointToPoint1.EnablePcapAll ("iperf-" + stack, false);

  apps.Start (Seconds (0.6));

  setPos (nodes.Get (0), 1, 10, 0);
  setPos (nodes.Get (1), 50,10, 0);
  setPos (nodes.Get (2), 100,10, 0);
  
  if(stack == "linux")
  {
    for ( float i = 0.6; i < 40.0 ; i=i+0.1)
   {
     GetSSStats(nodes.Get (0), Seconds(i), stack);
   }
  }

  Simulator::Stop (Seconds (40.0));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
