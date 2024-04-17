/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/flow-monitor-module.h"


namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

Time prevTime_send = Seconds (0);
Time prevTime_receive = Seconds (0);
Time prevTime_drop = Seconds (0);

Time PhyTxBegin = Seconds (0);
Time PhyTxEnd = Seconds (0);
Time PhyTxDrop = Seconds (0);
Time PhyRxDrop = Seconds (0);

uint64_t txBytes[7] = {0};
uint64_t rxBytes[7] = {0};
uint64_t DropBytes[7] = {0};
uint64_t prev_send[7] = {0};
uint64_t prev_receive[7] = {0};
uint64_t prev_drop[7] = {0};

uint64_t PhyTxBegintxBytes[7] = {0};
uint64_t PhyTxBeginprev[7] = {0};
uint64_t PhyTxEndtxBytes[7] = {0};
uint64_t PhyTxEndprev[7] = {0};
uint64_t PhyTxDropBytes[7] = {0};
uint64_t PhyTxDropprev[7] = {0};
uint64_t PhyRxDroprxBytes[7] = {0};
uint64_t PhyRxDroprxprev[7] = {0};
int32_t debug_trace = 0;


// static void
// TraceThroughput (Ptr<FlowMonitor> monitor)
// {
//     // std::cout << monitor << std::endl;

//   	FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
//  	std::map<FlowId, FlowMonitor::FlowStats>::const_iterator itr = stats.begin();  //检测第一条流
//  	//下边是最后一条流最后一条流
//  	// std::map<FlowId, FlowMonitor::FlowStats>::const_iterator itr = stats.end(); itr--;

//    	Time curTime = Now ();
//     std::cout << " test: " <<  curTime.GetSeconds () << " " << itr->second.txBytes << " " << prev_send << " " << itr->second.txBytes - prev_send << std::endl;
//     std::cout << "lxd debug:" <<  curTime << " " << 8 * 1072*(itr->second.txBytes - prev_send) / ((curTime.GetSeconds () - prevTime_send.GetSeconds ())) << std::endl;
//     prevTime_send = curTime;
//    	prev_send = itr->second.txBytes;
//    	Simulator::Schedule (Seconds (1), &TraceThroughput, monitor);
// }
static void
Tracedebug (){
  Time now = Simulator::Now();
  std::cout<<"time: "<<now<<std::endl;
}


static void
TraceThroughput_send ()
{
 
   	Time curTime = Now ();
    for(int i = 0 ; i<=5 ; i++){
    std::cout << " MacTx Send " <<  i << " " << txBytes[i] << " " << prev_send[i] << " " << txBytes[i] - prev_send[i] << std::endl;
    //std::cout << "send:" << " " <<  8 * (txBytes[i] - prev_send[i]) / ((curTime.GetSeconds () - prevTime_send.GetSeconds ()))/(1000*1000) << std::endl;
   	prev_send[i] = txBytes[i];
    if (i == 5)
      std::cout<<endl;
    }
    
    prevTime_send = curTime;

    for(int i = 0 ; i<=5 ; i++){
    std::cout << " PhyTx Send Begin " <<  i << " " << PhyTxBegintxBytes[i] << " " << PhyTxBeginprev[i] << " " << PhyTxBegintxBytes[i] - PhyTxBeginprev[i] << std::endl;
    //std::cout << "send:" << " " <<  8 * (txBytes[i] - prev_send[i]) / ((curTime.GetSeconds () - prevTime_send.GetSeconds ()))/(1000*1000) << std::endl;
   	PhyTxBeginprev[i] = PhyTxBegintxBytes[i];
    if (i == 5)
      std::cout<<endl;
    }

    for(int i = 0 ; i<=5 ; i++){
    std::cout << " PhyTx Send End " <<  i << " " << PhyTxEndtxBytes[i] << " " << PhyTxEndprev[i] << " " << PhyTxEndtxBytes[i] - PhyTxEndprev[i] << std::endl;
    //std::cout << "send:" << " " <<  8 * (txBytes[i] - prev_send[i]) / ((curTime.GetSeconds () - prevTime_send.GetSeconds ()))/(1000*1000) << std::endl;
   	PhyTxEndprev[i] = PhyTxEndtxBytes[i];
    if (i == 5)
      std::cout<<endl;
    }

   	Simulator::Schedule (Seconds (1), &TraceThroughput_send);
}

static void
TraceThroughput_receive ()
{
    // std::cout << monitor << std::endl;

  // FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
 	// std::map<FlowId, FlowMonitor::FlowStats>::const_iterator itr = stats.begin();  //检测第一条流
 	//下边是最后一条流最后一条流
 	// std::map<FlowId, FlowMonitor::FlowStats>::const_iterator itr = stats.end(); itr--;

   	Time curTime = Now ();
    for(int i = 0 ; i<=6 ; i++){
    std::cout << " test_receive: " <<  i << " " << rxBytes[i] << " " << prev_receive[i] << " " << rxBytes[i] - prev_receive[i] << std::endl;
    std::cout << "receive:" <<  curTime << " " <<  8 * (rxBytes[i] - prev_receive[i]) / ((curTime.GetSeconds () - prevTime_receive.GetSeconds ())) / (1000*1000) << std::endl;
    if (i == 6)
      std::cout<<endl;
   	prev_receive[i] = rxBytes[i];
    }
    prevTime_receive = curTime;
   	Simulator::Schedule (Seconds (1), &TraceThroughput_receive);
}

static void
TraceThroughput_drop ()
{

   	Time curTime = Now ();
    for(int i = 0 ; i<=5 ; i++){
    std::cout << " Mac Tx Drop: " <<  i << " " << DropBytes[i] << " " << prev_drop[i] << " " << DropBytes[i] - prev_drop[i] << std::endl;
    //std::cout << "drop:" <<  curTime << " " <<  8 * (DropBytes[i] - prev_drop[i]) / ((curTime.GetSeconds () - prevTime_drop.GetSeconds ())) / (1000*1000) << std::endl;
   	prev_drop[i] = DropBytes[i];
    if(i == 5)
      std::cout<<endl;
    }

    for(int i = 0 ; i<=5 ; i++){
    std::cout << " Phy Tx Drop: " <<  i << " " << PhyTxDropBytes[i] << " " << PhyTxDropprev[i] << " " << PhyTxDropBytes[i] - PhyTxDropprev[i] << std::endl;
    //std::cout << "drop:" <<  curTime << " " <<  8 * (DropBytes[i] - prev_drop[i]) / ((curTime.GetSeconds () - prevTime_drop.GetSeconds ())) / (1000*1000) << std::endl;
   	PhyTxDropprev[i] = PhyTxDropBytes[i];
    if(i == 5)
      std::cout<<endl;
    }

    for(int i = 0 ; i<=5 ; i++){
    std::cout << " Phy Rx Drop: " <<  i << " " << PhyRxDroprxBytes[i] << " " << PhyRxDroprxprev[i] << " " << PhyRxDroprxBytes[i] - PhyRxDroprxprev[i] << std::endl;
    //std::cout << "drop:" <<  curTime << " " <<  8 * (DropBytes[i] - prev_drop[i]) / ((curTime.GetSeconds () - prevTime_drop.GetSeconds ())) / (1000*1000) << std::endl;
   	PhyRxDroprxprev[i] = PhyRxDroprxBytes[i];
    if(i == 5)
      std::cout<<endl;
    }

    prevTime_drop = curTime;
   	Simulator::Schedule (Seconds (1), &TraceThroughput_drop);
}

void test(uint32_t v1, uint32_t v2, uint32_t v3){
  std::cout << v1 << " " << v2 << " " << v3 << std::endl;
}

class PcapWriter {
public:
  PcapWriter(const std::string& file)
  {
    PcapHelper helper;
    m_pcap = helper.CreateFile(file, std::ios::out, PcapHelper::DLT_PPP);
  }

  void
  TracePacket(Ptr<const Packet> packet)
  {
    static PppHeader pppHeader;
    pppHeader.SetProtocol(0x0077);

    m_pcap->Write(Simulator::Now(), pppHeader, packet);
  }

private:
  Ptr<PcapFileWrapper> m_pcap;
};

void TraceMacPacket(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    txBytes[nodeId] += packet->GetSize();
    //std::cout << "size: " << packet->GetSize() << " " << "devId: "<<devId << " nodeId " << nodeId<< "total size: "<< txBytes[nodeId]<< std::endl;
    //}
}

void TraceMacPacket_receive(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    rxBytes[nodeId] += packet->GetSize();
    //std::cout << "size: " << packet->GetSize() << " " << "devId: "<<devId << " nodeId " << nodeId<< "total size: "<< txBytes[nodeId]<< std::endl;
    //}
}

void TraceMacPacket_drop(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    DropBytes[nodeId] += packet->GetSize();
    //std::cout << "size: " << packet->GetSize() << " " << "devId: "<<devId << " nodeId " << nodeId<< "total size: "<< txBytes[nodeId]<< std::endl;
    //}
}

void TracePhyTxBegin(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
    PhyTxBegintxBytes[nodeId] += packet->GetSize();
} 

void TracePhyTxEnd(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
    PhyTxEndtxBytes[nodeId] += packet->GetSize();
} 

void TracePhyTxDrop(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
    PhyTxDropBytes[nodeId] += packet->GetSize();
} 

void TracePhyRxDrop(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
    PhyRxDroprxBytes[nodeId] += packet->GetSize();
} 

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Gbps"));
  //Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("20000p"));

  // FlowMonitorHelper flowmon;
  // flowmon.GetMonitor();
  // Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
  // monitor->Start(Time(0));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(7);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(2));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(3), nodes.Get(4));
  p2p.Install(nodes.Get(3), nodes.Get(5));
  p2p.Install(nodes.Get(2), nodes.Get(3));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(10000);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/best-route");

  // Installing applications

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerPcon");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix0");
  consumerHelper.SetAttribute("CcAlgorithm", StringValue("CUBIC")); // 10 interests a second
  consumerHelper.SetAttribute("ReactToCongestionMarks", BooleanValue(true));
  consumerHelper.SetAttribute("UseCubicFastConvergence", BooleanValue(true));
  //consumerHelper.SetAttribute("UseCwa", BooleanValue(true)); // 10 interests a second
  //consumerHelper.SetAttribute("Frequency", StringValue("1160")); // 10 interests a second
  
  //apps0.Start(Seconds(1.0));
    
  
  ndn::AppHelper consumerHelper1("ns3::ndn::ConsumerPcon");
  consumerHelper1.SetPrefix("/prefix1");
  //consumerHelper1.SetAttribute("Frequency", StringValue("1160")); // 10 interests a second
  consumerHelper1.SetAttribute("CcAlgorithm", StringValue("CUBIC")); // 10 interests a second
  consumerHelper1.SetAttribute("ReactToCongestionMarks", BooleanValue(true));
  consumerHelper1.SetAttribute("UseCubicFastConvergence", BooleanValue(true));
  auto apps1 = consumerHelper1.Install(nodes.Get(1));  
  auto apps0 = consumerHelper.Install(nodes.Get(0));  
  
                   // first node
  
  //apps0.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&test, 1));
  
  apps0.Stop(Seconds(15.0)); // stop the consumer app at 10 seconds mark    
  apps1.Stop(Seconds(15.0)); // stop the consumer app at 10 seconds mark

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prefix0");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(4)); // last node

  producerHelper.SetPrefix("/prefix1");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(5)); // last node

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndnGlobalRoutingHelper.AddOrigins("/prefix1", nodes.Get(5));
  ndnGlobalRoutingHelper.AddOrigins("/prefix0", nodes.Get(4));
  
  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();

  // FlowMonitorHelper flowmon;
  // // flowmon.GetMonitor();
  // Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

    // FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
    // std::map<FlowId, FlowMonitor::FlowStats>::const_iterator itr = stats.begin();  //检测第一条流
    // // itr->second.txBytes = 0;
    // std::cout << itr->second.txBytes << std::endl;


  //cout<<monitor << " " << flowmon.GetMonitor()<<endl;
  Simulator::Schedule (Seconds (1), &TraceThroughput_send);
  Simulator::Schedule (Seconds (1), &TraceThroughput_receive);
  Simulator::Schedule (Seconds (1), &TraceThroughput_drop);

  // PcapWriter trace("ndn-simple-trace11_new.pcap");
  // Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTx",
  //                               MakeCallback(&PcapWriter::TracePacket, &trace));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTxDetail",
                                MakeCallback(&TraceMacPacket));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxEndDetail",
                                MakeCallback(&TraceMacPacket_receive));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTxDropDetail",
                                MakeCallback(&TraceMacPacket_drop));     

  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxBeginDetail",
                                MakeCallback(&TracePhyTxBegin));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxEndDetail",
                                MakeCallback(&TracePhyTxEnd));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyTxDropDetail",
                                MakeCallback(&TracePhyTxDrop));     

  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxDropDetail",
                                MakeCallback(&TracePhyRxDrop));                                                             
                                                             
  Simulator::Schedule (Seconds (1), &TraceThroughput_send);
  Simulator::Stop(Seconds(15.0));    //一定要加这句话，否则会出现仿真不会停止的情况
  Simulator::Run ();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
