#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/flow-monitor-module.h"
//#include "ndn-load-balancer/random-load-balancer-strategy.hpp"
#include "ndn-load-balancer/random-load-balancer-strategy.cpp"
#include "ns3/ndnSIM/apps/ndn-ml.hpp"
//#include "random-strategy.cpp"
using ns3::ndn::GlobalRoutingHelper;
using ns3::ndn::StrategyChoiceHelper;
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

uint64_t allgatherBytes[40] = {0};
uint64_t txBytes[40] = {0};
uint64_t DropBytes[40] = {0};
uint32_t scattersign[40] = {0};
static int32_t debug_trace = 0;
static int32_t allgather_sign = 0;
static int32_t scatter_sign = 0;
static std::ofstream testFile("fattree_200M_timeout.txt");
static std::ofstream test_machine0("fattree_200M_send.txt");
static std::ofstream test_machine0_hop("fattree_200M_receive.txt");
static std::ofstream test_machine0_cubic("fattree_200M_cubicaction.txt");
static double total_hopcount = 0;
static double total_content = 0;
static int64_t packet_sent = 0;

static void
Tracedebug (){
  Time now = Simulator::Now();
  if(scatter_sign != 16){
    std::cout<<"time: "<<now<<"\tscatter packets for 0 : "<<debug_trace<<std::endl;
    std::cout<<"time: "<<now<<"\t0 send packets : "<<packet_sent<<std::endl;
    Simulator::Schedule(Seconds (0.001), &Tracedebug);
    
  }
  else{
    if(allgather_sign != 16)
    {
      std::cout<<"time: "<<now<<"\tallgather packets for 0 : "<<debug_trace<<std::endl;
       std::cout<<"time: "<<now<<"\t0 send packets : "<<packet_sent<<std::endl;
      Simulator::Schedule(Seconds (0.001), &Tracedebug);
    }
  }
}



static
void TraceMacPacket_drop(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    DropBytes[nodeId] += packet->GetSize();
    //std::cout << "drop size: " << packet->GetSize() << " " << "devId: "<<devId << " nodeId " << nodeId<< std::endl;
    //}
}
static
void TraceMacPacket(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    // txBytes[nodeId] += packet->GetSize();
    txBytes[nodeId] += 1;
    //std::cout << "send size: " << packet->GetSize() << " " << "devId: "<<devId << " nodeId " << nodeId<<std::endl;
    //}
}
static
void TraceMacPacket_receive(Ptr<const Packet> packet, uint32_t devId, uint32_t nodeId){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //rxBytes[nodeId] += packet->GetSize();
    //std::cout << "receive size: " << packet->GetSize() << " " << "devId: "<<devId << " nodeId " << nodeId<< std::endl;
    //}
}
static
void TraceAllgather_size(uint32_t nodeid, uint32_t pktsize){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    // allgatherBytes[nodeid] += pktsize;
    allgatherBytes[nodeid] += 1;
    if(nodeid == 0)
      debug_trace++;
    //}
}

static
void Tracescattersign(uint32_t nodeid){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
      std::cout<<"machine : "<<nodeid<<" finish scatter"<<", time: "<<Simulator::Now()<<std::endl;
      scatter_sign++;

    //}
}

static
void Traceallgathersign(uint32_t nodeid){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
    allgather_sign++;
    //}
}

static
void Tracetimeout(uint32_t nodeid,uint32_t seq,double m_window,double m_inFlight,Time rto){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
    testFile<<"node: "<<nodeid<<" time out for "<<seq<<", window: "<<m_window<<", inflight: "<<m_inFlight<<", RTO: "<<rto<<" , time: "<<Simulator::Now()<<std::endl;
    //}
}

static
void Retrans(uint32_t nodeid,uint32_t seq,double m_window,double m_inFlight,uint32_t m_seq){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
    testFile<<"node: "<<nodeid<<" retrans for "<<seq<<", window: "<<m_window<<", inflight: "<<m_inFlight<<" , time: "<<Simulator::Now()<<std::endl;
    //}
}

static
void Trace_send(bool retx_flag,uint32_t seq,double window,double inflight,uint32_t m_seq){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
    test_machine0<<"node: 0"<<" send for "<<seq<<", m_Seq: "<<m_seq<<" , retrans or not: "<<retx_flag<<", window: "<<window<<", inflight: "<<inflight<<" , time: "<<Simulator::Now()<<std::endl;
    packet_sent++;
    //}
}

static
void Trace_Hop(uint32_t seq,uint32_t hop,double m_window,double m_inFlight,uint64_t congestion,Time rtt){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
    test_machine0_hop<<"node: 0"<<" ondata for "<<seq<<", window: "<<m_window<<", inflight: "<<m_inFlight<<", RTT: "<<rtt<<", conmark: "<<congestion<<" , time: "<<Simulator::Now()<<", Hop count: "<<hop<<std::endl;
    total_hopcount += hop;
    total_content++;
    //}
}

static
void Trace_CubicIncrease(double m_window,double m_ssthresh){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
    test_machine0_cubic<<"node: 0"<<" window increase: "<<"m_window:"<<m_window<<", m_ssthresh: "<<m_ssthresh<<" , time: "<<Simulator::Now()<<std::endl;
    //}
}

static
void Trace_CubicDecrease(double m_window,double m_ssthresh,double m_cubicWmax,double m_cubicLastWmax){
  //if(nodeId == 0 || nodeId == 1 || nodeId == 4 || nodeId == 5 ){
    //DropBytes[nodeId] += packet->GetSize();
    //std::cout <<nodeid<<" send "<< "content size: " << pktsize << std::endl;
    //allgatherBytes[nodeid] += pktsize;
    test_machine0_cubic<<"node: 0"<<" window decrease: "<<"m_window:"<<m_window<<", m_ssthresh: "<<m_ssthresh<<", m_cubicWmax: "<<m_cubicWmax<<", m_cubicLastWmax: "<<m_cubicLastWmax<<" , time: "<<Simulator::Now()<<std::endl;
    //}
}

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("10Gbps"));
  //Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("100000p"));
  Config::SetDefault ("ns3::ndn::RttEstimator::MaxRTO", TimeValue (Seconds(0.03)));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(40);

  PointToPointHelper p2p;
   /*pod0*/
  p2p.Install(nodes.Get(0), nodes.Get(2));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  //p2p.Install(nodes.Get(0), nodes.Get(1));
  //p2p.Install(nodes.Get(3), nodes.Get(0));
  p2p.Install(nodes.Get(3), nodes.Get(4));
  p2p.Install(nodes.Get(3), nodes.Get(5));
  //p2p.Install(nodes.Get(2), nodes.Get(3));
  //p2p.Install(nodes.Get(1), nodes.Get(0));
  p2p.Install(nodes.Get(2), nodes.Get(6));
  p2p.Install(nodes.Get(2), nodes.Get(7));
  p2p.Install(nodes.Get(6), nodes.Get(3));
  p2p.Install(nodes.Get(7), nodes.Get(3));


  /*pod1*/
  p2p.Install(nodes.Get(10), nodes.Get(12));
  p2p.Install(nodes.Get(11), nodes.Get(12));
  //p2p.Install(nodes.Get(0), nodes.Get(1));
  //p2p.Install(nodes.Get(3), nodes.Get(0));
  p2p.Install(nodes.Get(13), nodes.Get(14));
  p2p.Install(nodes.Get(13), nodes.Get(15));
  //p2p.Install(nodes.Get(2), nodes.Get(3));
  //p2p.Install(nodes.Get(1), nodes.Get(0));
  p2p.Install(nodes.Get(12), nodes.Get(16));
  p2p.Install(nodes.Get(12), nodes.Get(17));
  p2p.Install(nodes.Get(16), nodes.Get(13));
  p2p.Install(nodes.Get(17), nodes.Get(13));

  /*pod2*/
  p2p.Install(nodes.Get(20), nodes.Get(22));
  p2p.Install(nodes.Get(21), nodes.Get(22));
  //p2p.Install(nodes.Get(0), nodes.Get(1));
  //p2p.Install(nodes.Get(3), nodes.Get(0));
  p2p.Install(nodes.Get(23), nodes.Get(24));
  p2p.Install(nodes.Get(23), nodes.Get(25));
  //p2p.Install(nodes.Get(2), nodes.Get(3));
  //p2p.Install(nodes.Get(1), nodes.Get(0));
  p2p.Install(nodes.Get(22), nodes.Get(26));
  p2p.Install(nodes.Get(22), nodes.Get(27));
  p2p.Install(nodes.Get(26), nodes.Get(23));
  p2p.Install(nodes.Get(27), nodes.Get(23));

  /*pod3*/
  p2p.Install(nodes.Get(30), nodes.Get(32));
  p2p.Install(nodes.Get(31), nodes.Get(32));
  //p2p.Install(nodes.Get(0), nodes.Get(1));
  //p2p.Install(nodes.Get(3), nodes.Get(0));
  p2p.Install(nodes.Get(33), nodes.Get(34));
  p2p.Install(nodes.Get(33), nodes.Get(35));
  //p2p.Install(nodes.Get(2), nodes.Get(3));
  //p2p.Install(nodes.Get(1), nodes.Get(0));
  p2p.Install(nodes.Get(32), nodes.Get(36));
  p2p.Install(nodes.Get(32), nodes.Get(37));
  p2p.Install(nodes.Get(36), nodes.Get(33));
  p2p.Install(nodes.Get(37), nodes.Get(33));

  /*toplayer*/
  // p2p.Install(nodes.Get(6), nodes.Get(8));
  // p2p.Install(nodes.Get(16), nodes.Get(8));
  // p2p.Install(nodes.Get(26), nodes.Get(8));
  // p2p.Install(nodes.Get(36), nodes.Get(8));
  // // p2p.Install(nodes.Get(6), nodes.Get(9));
  // p2p.Install(nodes.Get(6), nodes.Get(18));
  // p2p.Install(nodes.Get(16), nodes.Get(18));
  // p2p.Install(nodes.Get(26), nodes.Get(18));
  // p2p.Install(nodes.Get(36), nodes.Get(18));
  // //p2p.Install(nodes.Get(7), nodes.Get(8));
  // //p2p.Install(nodes.Get(18), nodes.Get(16));
  // p2p.Install(nodes.Get(7), nodes.Get(28));
  // p2p.Install(nodes.Get(17), nodes.Get(28));
  // p2p.Install(nodes.Get(27), nodes.Get(28));
  // p2p.Install(nodes.Get(37), nodes.Get(28));

  // p2p.Install(nodes.Get(7), nodes.Get(38));
  // p2p.Install(nodes.Get(17), nodes.Get(38));
  // p2p.Install(nodes.Get(27), nodes.Get(38));
  // p2p.Install(nodes.Get(37), nodes.Get(38));
  p2p.Install(nodes.Get(6), nodes.Get(8));
  p2p.Install(nodes.Get(7), nodes.Get(9));
  p2p.Install(nodes.Get(16), nodes.Get(8));
  p2p.Install(nodes.Get(17), nodes.Get(9));
  p2p.Install(nodes.Get(26), nodes.Get(8));
  p2p.Install(nodes.Get(27), nodes.Get(9));
  p2p.Install(nodes.Get(36), nodes.Get(8));
  p2p.Install(nodes.Get(37), nodes.Get(9));

  p2p.Install(nodes.Get(6), nodes.Get(18));
  p2p.Install(nodes.Get(7), nodes.Get(19));
  p2p.Install(nodes.Get(16), nodes.Get(18));
  p2p.Install(nodes.Get(17), nodes.Get(19));
  p2p.Install(nodes.Get(26), nodes.Get(18));
  p2p.Install(nodes.Get(27), nodes.Get(19));
  p2p.Install(nodes.Get(36), nodes.Get(18));
  p2p.Install(nodes.Get(37), nodes.Get(19));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(10000);
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.InstallAll();
  // ndnHelper.setCsSize(100000);
  // NodeContainer cache_nodes;
  // cache_nodes.Add(nodes.Get(2));
  // cache_nodes.Add(nodes.Get(3));
  // cache_nodes.Add(nodes.Get(12));
  // cache_nodes.Add(nodes.Get(13));
  // cache_nodes.Add(nodes.Get(22));
  // cache_nodes.Add(nodes.Get(23));
  // cache_nodes.Add(nodes.Get(32));
  // cache_nodes.Add(nodes.Get(33));
  // ndnHelper.Install(cache_nodes);

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(2),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(3),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(12),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(13),"/");
  ndn::StrategyChoiceHelper::Install(nodes.Get(2),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(3),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(12),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(13),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(22),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(23),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(32),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(33),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(7),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(17),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(6),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(7),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(16),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(17),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(26),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(27),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(36),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(37),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(16),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(26),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(27),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(36),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(37),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(6),"/2");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(6),"/2");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(6),"/3");
  //ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomStrategy>(nodes.Get(7),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(16),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(17),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(16),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(17),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(8),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(9),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(2),"/3");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(3),"/1");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(3),"/0");

  ndn::AppHelper producerHelper("ns3::ndn::ndnml");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/2");
  //producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("MachineRank",StringValue("2"));
  producerHelper.SetAttribute("TotalMachineNumber",StringValue("16"));
  producerHelper.SetAttribute("ModelSize",StringValue("12000"));
  ndnGlobalRoutingHelper.AddOrigins("/2", nodes.Get(4));
  auto apps2 = producerHelper.Install(nodes.Get(4)); // last node
  //apps2.Stop(Seconds(20.0)); // stop the consumer app at 10 seconds mark  

  ndn::AppHelper consumerHelper("ns3::ndn::ndnml");
  consumerHelper.SetPrefix("/0");
  consumerHelper.SetAttribute("MachineRank",StringValue("0"));
  consumerHelper.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/0", nodes.Get(0));
  auto apps0 = consumerHelper.Install(nodes.Get(0));  
  //apps0.Stop(Seconds(20.0)); // stop the consumer app at 10 seconds mark   


  ndn::AppHelper consumerHelper1("ns3::ndn::ndnml");
  consumerHelper1.SetPrefix("/1");
  consumerHelper1.SetAttribute("MachineRank",StringValue("1"));
  consumerHelper1.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper1.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/1", nodes.Get(1));
  auto apps1 = consumerHelper1.Install(nodes.Get(1));  
  //apps1.Stop(Seconds(20.0)); // stop the consumer app at 10 seconds mark   

  ndn::AppHelper consumerHelper2("ns3::ndn::ndnml");
  consumerHelper2.SetPrefix("/3");
  consumerHelper2.SetAttribute("MachineRank",StringValue("3"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/3", nodes.Get(5));
  auto apps3 = consumerHelper2.Install(nodes.Get(5));   

  consumerHelper2.SetPrefix("/4");
  consumerHelper2.SetAttribute("MachineRank",StringValue("4"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/4", nodes.Get(10));
  auto apps4 = consumerHelper2.Install(nodes.Get(10));   

  consumerHelper2.SetPrefix("/5");
  consumerHelper2.SetAttribute("MachineRank",StringValue("5"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/5", nodes.Get(11));
  auto apps5 = consumerHelper2.Install(nodes.Get(11));   
  
  consumerHelper2.SetPrefix("/6");
  consumerHelper2.SetAttribute("MachineRank",StringValue("6"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/6", nodes.Get(14));
  auto apps6 = consumerHelper2.Install(nodes.Get(14));   

  consumerHelper2.SetPrefix("/7");
  consumerHelper2.SetAttribute("MachineRank",StringValue("7"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/7", nodes.Get(15));
  auto apps7 = consumerHelper2.Install(nodes.Get(15));   

  consumerHelper2.SetPrefix("/8");
  consumerHelper2.SetAttribute("MachineRank",StringValue("8"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/8", nodes.Get(20));
  auto apps8 = consumerHelper2.Install(nodes.Get(20));  

  consumerHelper2.SetPrefix("/9");
  consumerHelper2.SetAttribute("MachineRank",StringValue("9"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/9", nodes.Get(21));
  auto apps9 = consumerHelper2.Install(nodes.Get(21));  

  consumerHelper2.SetPrefix("/10");
  consumerHelper2.SetAttribute("MachineRank",StringValue("10"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/10", nodes.Get(24));
  auto apps10 = consumerHelper2.Install(nodes.Get(24));  

  consumerHelper2.SetPrefix("/11");
  consumerHelper2.SetAttribute("MachineRank",StringValue("11"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/11", nodes.Get(25));
  auto apps11 = consumerHelper2.Install(nodes.Get(25));  

  consumerHelper2.SetPrefix("/12");
  consumerHelper2.SetAttribute("MachineRank",StringValue("12"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/12", nodes.Get(30));
  auto apps12 = consumerHelper2.Install(nodes.Get(30));  

  consumerHelper2.SetPrefix("/13");
  consumerHelper2.SetAttribute("MachineRank",StringValue("13"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/13", nodes.Get(31));
  auto apps13 = consumerHelper2.Install(nodes.Get(31));  

  consumerHelper2.SetPrefix("/14");
  consumerHelper2.SetAttribute("MachineRank",StringValue("14"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/14", nodes.Get(34));
  auto apps14 = consumerHelper2.Install(nodes.Get(34));  

  consumerHelper2.SetPrefix("/15");
  consumerHelper2.SetAttribute("MachineRank",StringValue("15"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("12000"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("16"));
  ndnGlobalRoutingHelper.AddOrigins("/15", nodes.Get(35));
  auto apps15 = consumerHelper2.Install(nodes.Get(35));  

  // u_int16_t ndnml::scatfin_machine = 0;
  //u_int16_t ndnml::all_scatter_fin = 0;
  //Calculate and install FIBs
  GlobalRoutingHelper::CalculateLfidRoutes();

  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTxDropDetail",
                                MakeCallback(&TraceMacPacket_drop));  
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/MacTxDetail",
                                MakeCallback(&TraceMacPacket));
  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::PointToPointNetDevice/PhyRxEndDetail",
                                MakeCallback(&TraceMacPacket_receive));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/Allgathersize",
                                MakeCallback(&TraceAllgather_size));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/Scatterfinishsign",
                                MakeCallback(&Tracescattersign));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/Allgatherfinishsign",
                                MakeCallback(&Traceallgathersign));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/TraceTimeout",
                                MakeCallback(&Tracetimeout));
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/TraceRetrans",
                                MakeCallback(&Retrans));  
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/TraceSend",
                                MakeCallback(&Trace_send));  
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/TraceHop",
                                MakeCallback(&Trace_Hop)); 
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/TraceCubicIncrease",
                                MakeCallback(&Trace_CubicIncrease)); 
  Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::ndn::ndnml/TraceCubicDecrease",
                                MakeCallback(&Trace_CubicDecrease));                                                        
  Simulator::Schedule (Seconds (0.001), &Tracedebug);

  Simulator::Stop(Seconds(10.0));    //一定要加这句话，否则会出现仿真不会停止的情况
  Simulator::Run ();
  Simulator::Destroy();
  for(int i = 0; i < 40; i++){
    std::cout << "node "<<i<<" send "<< "content size: " << allgatherBytes[i] << std::endl;
  }
  for(int i = 0; i < 40; i++){
    std::cout << "node "<<i<<" send "<< "packet num: " << txBytes[i] << std::endl;
  }
  for(int i = 0; i < 40; i++){
    std::cout << "node "<<i<<" drop "<< "packet size: " << DropBytes[i] << std::endl;
  }
  test_machine0_hop<<"average hop count: "<<total_hopcount/total_content<<std::endl;
  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
