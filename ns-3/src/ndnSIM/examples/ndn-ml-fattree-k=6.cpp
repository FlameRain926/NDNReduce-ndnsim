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

uint64_t allgatherBytes[100] = {0};
uint64_t txBytes[100] = {0};
uint64_t DropBytes[100] = {0};
uint32_t scattersign[100] = {0};
static int32_t debug_trace = 0;
static int32_t allgather_sign = 0;
static int32_t scatter_sign = 0;
static std::ofstream testFile("new_fattree_54_200M_1_timeout.txt");
static std::ofstream test_machine0("new_fattree_54_200M_1_send.txt");
static std::ofstream test_machine0_hop("new_fattree_54_200M_1_receive.txt");
static std::ofstream test_machine0_cubic("new_fattree_54_200M_1_cubicaction.txt");
static double total_hopcount = 0;
static double total_content = 0;
static int64_t packet_sent = 0;

static void
Tracedebug (){
  Time now = Simulator::Now();
  if(scatter_sign != 54){
    std::cout<<"time: "<<now<<"\tscatter packets for 0 : "<<debug_trace<<std::endl;
    std::cout<<"time: "<<now<<"\t0 send packets : "<<packet_sent<<std::endl;
    Simulator::Schedule(Seconds (0.001), &Tracedebug);
    
  }
  else{
    if(allgather_sign != 54)
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
  Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", StringValue("10000p"));
  Config::SetDefault ("ns3::ndn::RttEstimator::MaxRTO", TimeValue (Seconds(0.03)));

  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(100);

  PointToPointHelper p2p;
   /*pod0*/
  p2p.Install(nodes.Get(0), nodes.Get(54));
  p2p.Install(nodes.Get(1), nodes.Get(54));
  p2p.Install(nodes.Get(2), nodes.Get(54));

  p2p.Install(nodes.Get(3), nodes.Get(55));
  p2p.Install(nodes.Get(4), nodes.Get(55));
  p2p.Install(nodes.Get(5), nodes.Get(55));

  p2p.Install(nodes.Get(6), nodes.Get(56));
  p2p.Install(nodes.Get(7), nodes.Get(56));
  p2p.Install(nodes.Get(8), nodes.Get(56));

  p2p.Install(nodes.Get(54), nodes.Get(57));
  p2p.Install(nodes.Get(54), nodes.Get(58));
  p2p.Install(nodes.Get(54), nodes.Get(59));

  p2p.Install(nodes.Get(55), nodes.Get(57));
  p2p.Install(nodes.Get(55), nodes.Get(58));
  p2p.Install(nodes.Get(55), nodes.Get(59));

  p2p.Install(nodes.Get(56), nodes.Get(57));
  p2p.Install(nodes.Get(56), nodes.Get(58));
  p2p.Install(nodes.Get(56), nodes.Get(59));



    /*pod1*/
  p2p.Install(nodes.Get(9), nodes.Get(60));
  p2p.Install(nodes.Get(10), nodes.Get(60));
  p2p.Install(nodes.Get(11), nodes.Get(60));

  p2p.Install(nodes.Get(12), nodes.Get(61));
  p2p.Install(nodes.Get(13), nodes.Get(61));
  p2p.Install(nodes.Get(14), nodes.Get(61));

  p2p.Install(nodes.Get(15), nodes.Get(62));
  p2p.Install(nodes.Get(16), nodes.Get(62));
  p2p.Install(nodes.Get(17), nodes.Get(62));

  p2p.Install(nodes.Get(60), nodes.Get(63));
  p2p.Install(nodes.Get(60), nodes.Get(64));
  p2p.Install(nodes.Get(60), nodes.Get(65));

  p2p.Install(nodes.Get(61), nodes.Get(63));
  p2p.Install(nodes.Get(61), nodes.Get(64));
  p2p.Install(nodes.Get(61), nodes.Get(65));

  p2p.Install(nodes.Get(62), nodes.Get(63));
  p2p.Install(nodes.Get(62), nodes.Get(64));
  p2p.Install(nodes.Get(62), nodes.Get(65));

    /*pod2*/
  p2p.Install(nodes.Get(18), nodes.Get(66));
  p2p.Install(nodes.Get(19), nodes.Get(66));
  p2p.Install(nodes.Get(20), nodes.Get(66));

  p2p.Install(nodes.Get(21), nodes.Get(67));
  p2p.Install(nodes.Get(22), nodes.Get(67));
  p2p.Install(nodes.Get(23), nodes.Get(67));

  p2p.Install(nodes.Get(24), nodes.Get(68));
  p2p.Install(nodes.Get(25), nodes.Get(68));
  p2p.Install(nodes.Get(26), nodes.Get(68));

  p2p.Install(nodes.Get(66), nodes.Get(69));
  p2p.Install(nodes.Get(66), nodes.Get(70));
  p2p.Install(nodes.Get(66), nodes.Get(71));

  p2p.Install(nodes.Get(67), nodes.Get(69));
  p2p.Install(nodes.Get(67), nodes.Get(70));
  p2p.Install(nodes.Get(67), nodes.Get(71));

  p2p.Install(nodes.Get(68), nodes.Get(69));
  p2p.Install(nodes.Get(68), nodes.Get(70));
  p2p.Install(nodes.Get(68), nodes.Get(71));

    /*pod3*/
  p2p.Install(nodes.Get(27), nodes.Get(72));
  p2p.Install(nodes.Get(28), nodes.Get(72));
  p2p.Install(nodes.Get(29), nodes.Get(72));

  p2p.Install(nodes.Get(30), nodes.Get(73));
  p2p.Install(nodes.Get(31), nodes.Get(73));
  p2p.Install(nodes.Get(32), nodes.Get(73));

  p2p.Install(nodes.Get(33), nodes.Get(74));
  p2p.Install(nodes.Get(34), nodes.Get(74));
  p2p.Install(nodes.Get(35), nodes.Get(74));

  p2p.Install(nodes.Get(72), nodes.Get(75));
  p2p.Install(nodes.Get(72), nodes.Get(76));
  p2p.Install(nodes.Get(72), nodes.Get(77));

  p2p.Install(nodes.Get(73), nodes.Get(75));
  p2p.Install(nodes.Get(73), nodes.Get(76));
  p2p.Install(nodes.Get(73), nodes.Get(77));

  p2p.Install(nodes.Get(74), nodes.Get(75));
  p2p.Install(nodes.Get(74), nodes.Get(76));
  p2p.Install(nodes.Get(74), nodes.Get(77));

      /*pod4*/
  p2p.Install(nodes.Get(36), nodes.Get(78));
  p2p.Install(nodes.Get(37), nodes.Get(78));
  p2p.Install(nodes.Get(38), nodes.Get(78));

  p2p.Install(nodes.Get(39), nodes.Get(79));
  p2p.Install(nodes.Get(40), nodes.Get(79));
  p2p.Install(nodes.Get(41), nodes.Get(79));

  p2p.Install(nodes.Get(42), nodes.Get(80));
  p2p.Install(nodes.Get(43), nodes.Get(80));
  p2p.Install(nodes.Get(44), nodes.Get(80));

  p2p.Install(nodes.Get(78), nodes.Get(81));
  p2p.Install(nodes.Get(78), nodes.Get(82));
  p2p.Install(nodes.Get(78), nodes.Get(83));

  p2p.Install(nodes.Get(79), nodes.Get(81));
  p2p.Install(nodes.Get(79), nodes.Get(82));
  p2p.Install(nodes.Get(79), nodes.Get(83));

  p2p.Install(nodes.Get(80), nodes.Get(81));
  p2p.Install(nodes.Get(80), nodes.Get(82));
  p2p.Install(nodes.Get(80), nodes.Get(83));

        /*pod5*/
  p2p.Install(nodes.Get(45), nodes.Get(84));
  p2p.Install(nodes.Get(46), nodes.Get(84));
  p2p.Install(nodes.Get(47), nodes.Get(84));

  p2p.Install(nodes.Get(48), nodes.Get(85));
  p2p.Install(nodes.Get(49), nodes.Get(85));
  p2p.Install(nodes.Get(50), nodes.Get(85));

  p2p.Install(nodes.Get(51), nodes.Get(86));
  p2p.Install(nodes.Get(52), nodes.Get(86));
  p2p.Install(nodes.Get(53), nodes.Get(86));

  p2p.Install(nodes.Get(84), nodes.Get(87));
  p2p.Install(nodes.Get(84), nodes.Get(88));
  p2p.Install(nodes.Get(84), nodes.Get(89));

  p2p.Install(nodes.Get(85), nodes.Get(87));
  p2p.Install(nodes.Get(85), nodes.Get(88));
  p2p.Install(nodes.Get(85), nodes.Get(89));

  p2p.Install(nodes.Get(86), nodes.Get(87));
  p2p.Install(nodes.Get(86), nodes.Get(88));
  p2p.Install(nodes.Get(86), nodes.Get(89));


  //aggregation
  p2p.Install(nodes.Get(57), nodes.Get(92));
  p2p.Install(nodes.Get(57), nodes.Get(91));
  p2p.Install(nodes.Get(57), nodes.Get(90));
  p2p.Install(nodes.Get(58), nodes.Get(93));
  p2p.Install(nodes.Get(58), nodes.Get(94));
  p2p.Install(nodes.Get(58), nodes.Get(95));
  p2p.Install(nodes.Get(59), nodes.Get(96));
  p2p.Install(nodes.Get(59), nodes.Get(97));
  p2p.Install(nodes.Get(59), nodes.Get(98));

  p2p.Install(nodes.Get(63), nodes.Get(92));
  p2p.Install(nodes.Get(63), nodes.Get(91));
  p2p.Install(nodes.Get(63), nodes.Get(90));
  p2p.Install(nodes.Get(64), nodes.Get(93));
  p2p.Install(nodes.Get(64), nodes.Get(94));
  p2p.Install(nodes.Get(64), nodes.Get(95));
  p2p.Install(nodes.Get(65), nodes.Get(96));
  p2p.Install(nodes.Get(65), nodes.Get(97));
  p2p.Install(nodes.Get(65), nodes.Get(98));

  p2p.Install(nodes.Get(69), nodes.Get(92));
  p2p.Install(nodes.Get(69), nodes.Get(91));
  p2p.Install(nodes.Get(69), nodes.Get(90));
  p2p.Install(nodes.Get(70), nodes.Get(94));
  p2p.Install(nodes.Get(70), nodes.Get(93));
  p2p.Install(nodes.Get(70), nodes.Get(95));
  p2p.Install(nodes.Get(71), nodes.Get(97));
  p2p.Install(nodes.Get(71), nodes.Get(96));
  p2p.Install(nodes.Get(71), nodes.Get(98));

  p2p.Install(nodes.Get(75), nodes.Get(92));
  p2p.Install(nodes.Get(75), nodes.Get(91));
  p2p.Install(nodes.Get(75), nodes.Get(90));
  p2p.Install(nodes.Get(76), nodes.Get(94));
  p2p.Install(nodes.Get(76), nodes.Get(93));
  p2p.Install(nodes.Get(76), nodes.Get(95));
  p2p.Install(nodes.Get(77), nodes.Get(97));
  p2p.Install(nodes.Get(77), nodes.Get(96));
  p2p.Install(nodes.Get(77), nodes.Get(98));

  p2p.Install(nodes.Get(81), nodes.Get(92));
  p2p.Install(nodes.Get(81), nodes.Get(90));
  p2p.Install(nodes.Get(81), nodes.Get(91));
  p2p.Install(nodes.Get(82), nodes.Get(95));
  p2p.Install(nodes.Get(82), nodes.Get(94));
  p2p.Install(nodes.Get(82), nodes.Get(93));
  p2p.Install(nodes.Get(83), nodes.Get(98));
  p2p.Install(nodes.Get(83), nodes.Get(97));
  p2p.Install(nodes.Get(83), nodes.Get(96));

  p2p.Install(nodes.Get(87), nodes.Get(92));
  p2p.Install(nodes.Get(87), nodes.Get(90));
  p2p.Install(nodes.Get(87), nodes.Get(91));
  p2p.Install(nodes.Get(88), nodes.Get(95));
  p2p.Install(nodes.Get(88), nodes.Get(94));
  p2p.Install(nodes.Get(88), nodes.Get(93));
  p2p.Install(nodes.Get(89), nodes.Get(98));
  p2p.Install(nodes.Get(89), nodes.Get(97));
  p2p.Install(nodes.Get(89), nodes.Get(96));


  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(30000);
  ndnHelper.setPolicy("nfd::cs::lru");
  ndnHelper.InstallAll();

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();

  // Choosing forwarding strategy
  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/best-route");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(2),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(3),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(12),"/");
  // ndn::StrategyChoiceHelper::Install<ndn::nfd::fw::RandomLoadBalancerStrategy>(nodes.Get(13),"/");

  ndn::StrategyChoiceHelper::Install(nodes.Get(54),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(55),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(56),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(60),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(61),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(62),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(66),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(67),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(68),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(72),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(73),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(74),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(78),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(79),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(80),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(84),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(85),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(86),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(57),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(58),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(59),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(63),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(64),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(65),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(69),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(70),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(71),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(75),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(76),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(77),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(81),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(82),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(83),"/","/localhost/nfd/strategy/random");

  ndn::StrategyChoiceHelper::Install(nodes.Get(87),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(88),"/","/localhost/nfd/strategy/random");
  ndn::StrategyChoiceHelper::Install(nodes.Get(89),"/","/localhost/nfd/strategy/random");


  // ndn::StrategyChoiceHelper::Install(nodes.Get(7),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(17),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(6),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(7),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(16),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(17),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(26),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(27),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(36),"/","/localhost/nfd/strategy/random");
//   ndn::StrategyChoiceHelper::Install(nodes.Get(37),"/","/localhost/nfd/strategy/random");
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



  ndn::AppHelper consumerHelper("ns3::ndn::ndnml");
  consumerHelper.SetPrefix("/0");
  consumerHelper.SetAttribute("MachineRank",StringValue("0"));
  consumerHelper.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/0", nodes.Get(0));
  auto apps0 = consumerHelper.Install(nodes.Get(0));  
  //apps0.Stop(Seconds(20.0)); // stop the consumer app at 10 seconds mark   


  ndn::AppHelper consumerHelper1("ns3::ndn::ndnml");
  consumerHelper1.SetPrefix("/1");
  consumerHelper1.SetAttribute("MachineRank",StringValue("1"));
  consumerHelper1.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper1.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/1", nodes.Get(1));
  auto apps1 = consumerHelper1.Install(nodes.Get(1));  
  //apps1.Stop(Seconds(20.0)); // stop the consumer app at 10 seconds mark 

  consumerHelper1.SetPrefix("/2");
  consumerHelper1.SetAttribute("MachineRank",StringValue("2"));
  consumerHelper1.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper1.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/2", nodes.Get(2));
  auto apps2 = consumerHelper1.Install(nodes.Get(2));    

  ndn::AppHelper consumerHelper2("ns3::ndn::ndnml");
  consumerHelper2.SetPrefix("/3");
  consumerHelper2.SetAttribute("MachineRank",StringValue("3"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/3", nodes.Get(3));
  auto apps3 = consumerHelper2.Install(nodes.Get(3));   

  consumerHelper2.SetPrefix("/4");
  consumerHelper2.SetAttribute("MachineRank",StringValue("4"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/4", nodes.Get(4));
  auto apps4 = consumerHelper2.Install(nodes.Get(4));   

  consumerHelper2.SetPrefix("/5");
  consumerHelper2.SetAttribute("MachineRank",StringValue("5"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/5", nodes.Get(5));
  auto apps5 = consumerHelper2.Install(nodes.Get(5));   
  
  consumerHelper2.SetPrefix("/6");
  consumerHelper2.SetAttribute("MachineRank",StringValue("6"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/6", nodes.Get(6));
  auto apps6 = consumerHelper2.Install(nodes.Get(6));   

  consumerHelper2.SetPrefix("/7");
  consumerHelper2.SetAttribute("MachineRank",StringValue("7"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/7", nodes.Get(7));
  auto apps7 = consumerHelper2.Install(nodes.Get(7));   

  consumerHelper2.SetPrefix("/8");
  consumerHelper2.SetAttribute("MachineRank",StringValue("8"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/8", nodes.Get(8));
  auto apps8 = consumerHelper2.Install(nodes.Get(8));  

  consumerHelper2.SetPrefix("/9");
  consumerHelper2.SetAttribute("MachineRank",StringValue("9"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/9", nodes.Get(9));
  auto apps9 = consumerHelper2.Install(nodes.Get(9));  

  consumerHelper2.SetPrefix("/10");
  consumerHelper2.SetAttribute("MachineRank",StringValue("10"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/10", nodes.Get(10));
  auto apps10 = consumerHelper2.Install(nodes.Get(10));  

  consumerHelper2.SetPrefix("/11");
  consumerHelper2.SetAttribute("MachineRank",StringValue("11"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/11", nodes.Get(11));
  auto apps11 = consumerHelper2.Install(nodes.Get(11));  

  consumerHelper2.SetPrefix("/12");
  consumerHelper2.SetAttribute("MachineRank",StringValue("12"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/12", nodes.Get(12));
  auto apps12 = consumerHelper2.Install(nodes.Get(12));  

  consumerHelper2.SetPrefix("/13");
  consumerHelper2.SetAttribute("MachineRank",StringValue("13"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/13", nodes.Get(13));
  auto apps13 = consumerHelper2.Install(nodes.Get(13));  

  consumerHelper2.SetPrefix("/14");
  consumerHelper2.SetAttribute("MachineRank",StringValue("14"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/14", nodes.Get(14));
  auto apps14 = consumerHelper2.Install(nodes.Get(14));  

  consumerHelper2.SetPrefix("/15");
  consumerHelper2.SetAttribute("MachineRank",StringValue("15"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/15", nodes.Get(15));
  auto apps15 = consumerHelper2.Install(nodes.Get(15));  

  consumerHelper2.SetPrefix("/16");
  consumerHelper2.SetAttribute("MachineRank",StringValue("16"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/16", nodes.Get(16));
  auto apps16 = consumerHelper2.Install(nodes.Get(16));  

  consumerHelper2.SetPrefix("/17");
  consumerHelper2.SetAttribute("MachineRank",StringValue("17"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/17", nodes.Get(17));
  auto apps17 = consumerHelper2.Install(nodes.Get(17));  

  consumerHelper2.SetPrefix("/18");
  consumerHelper2.SetAttribute("MachineRank",StringValue("18"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/18", nodes.Get(18));
  auto apps18 = consumerHelper2.Install(nodes.Get(18));  

  consumerHelper2.SetPrefix("/19");
  consumerHelper2.SetAttribute("MachineRank",StringValue("19"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/19", nodes.Get(19));
  auto apps19 = consumerHelper2.Install(nodes.Get(19));  

  consumerHelper2.SetPrefix("/20");
  consumerHelper2.SetAttribute("MachineRank",StringValue("20"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/20", nodes.Get(20));
  auto apps20 = consumerHelper2.Install(nodes.Get(20));  

  consumerHelper2.SetPrefix("/21");
  consumerHelper2.SetAttribute("MachineRank",StringValue("21"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/21", nodes.Get(21));
  auto apps21 = consumerHelper2.Install(nodes.Get(21));  

  consumerHelper2.SetPrefix("/22");
  consumerHelper2.SetAttribute("MachineRank",StringValue("22"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/22", nodes.Get(22));
  auto apps22 = consumerHelper2.Install(nodes.Get(22));  

  consumerHelper2.SetPrefix("/23");
  consumerHelper2.SetAttribute("MachineRank",StringValue("23"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/23", nodes.Get(23));
  auto apps23 = consumerHelper2.Install(nodes.Get(23));  

  consumerHelper2.SetPrefix("/24");
  consumerHelper2.SetAttribute("MachineRank",StringValue("24"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/24", nodes.Get(24));
  auto apps24 = consumerHelper2.Install(nodes.Get(24));  

  consumerHelper2.SetPrefix("/25");
  consumerHelper2.SetAttribute("MachineRank",StringValue("25"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/25", nodes.Get(25));
  auto apps25 = consumerHelper2.Install(nodes.Get(25));  

  consumerHelper2.SetPrefix("/26");
  consumerHelper2.SetAttribute("MachineRank",StringValue("26"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/26", nodes.Get(26));
  auto apps26 = consumerHelper2.Install(nodes.Get(26));  

  consumerHelper2.SetPrefix("/27");
  consumerHelper2.SetAttribute("MachineRank",StringValue("27"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/27", nodes.Get(27));
  auto apps27 = consumerHelper2.Install(nodes.Get(27));  

  consumerHelper2.SetPrefix("/28");
  consumerHelper2.SetAttribute("MachineRank",StringValue("28"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/28", nodes.Get(28));
  auto apps28 = consumerHelper2.Install(nodes.Get(28));  

  consumerHelper2.SetPrefix("/29");
  consumerHelper2.SetAttribute("MachineRank",StringValue("29"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/29", nodes.Get(29));
  auto apps29 = consumerHelper2.Install(nodes.Get(29));  

  consumerHelper2.SetPrefix("/30");
  consumerHelper2.SetAttribute("MachineRank",StringValue("30"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/30", nodes.Get(30));
  auto apps30 = consumerHelper2.Install(nodes.Get(30));

  consumerHelper2.SetPrefix("/31");
  consumerHelper2.SetAttribute("MachineRank",StringValue("31"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/31", nodes.Get(31));
  auto apps31 = consumerHelper2.Install(nodes.Get(31));  

  consumerHelper2.SetPrefix("/32");
  consumerHelper2.SetAttribute("MachineRank",StringValue("32"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/32", nodes.Get(32));
  auto apps32 = consumerHelper2.Install(nodes.Get(32));  

  consumerHelper2.SetPrefix("/33");
  consumerHelper2.SetAttribute("MachineRank",StringValue("33"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/33", nodes.Get(33));
  auto apps33 = consumerHelper2.Install(nodes.Get(33));  

  consumerHelper2.SetPrefix("/34");
  consumerHelper2.SetAttribute("MachineRank",StringValue("34"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/34", nodes.Get(34));
  auto apps34 = consumerHelper2.Install(nodes.Get(34));  

  consumerHelper2.SetPrefix("/35");
  consumerHelper2.SetAttribute("MachineRank",StringValue("35"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/35", nodes.Get(35));
  auto apps35 = consumerHelper2.Install(nodes.Get(35));  

  consumerHelper2.SetPrefix("/36");
  consumerHelper2.SetAttribute("MachineRank",StringValue("36"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/36", nodes.Get(36));
  auto apps36 = consumerHelper2.Install(nodes.Get(36));  

  consumerHelper2.SetPrefix("/37");
  consumerHelper2.SetAttribute("MachineRank",StringValue("37"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/37", nodes.Get(37));
  auto apps37 = consumerHelper2.Install(nodes.Get(37));  

  consumerHelper2.SetPrefix("/38");
  consumerHelper2.SetAttribute("MachineRank",StringValue("38"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/38", nodes.Get(38));
  auto apps38 = consumerHelper2.Install(nodes.Get(38));  

  consumerHelper2.SetPrefix("/39");
  consumerHelper2.SetAttribute("MachineRank",StringValue("39"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/39", nodes.Get(39));
  auto apps39 = consumerHelper2.Install(nodes.Get(39));  

  consumerHelper2.SetPrefix("/40");
  consumerHelper2.SetAttribute("MachineRank",StringValue("40"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/40", nodes.Get(40));
  auto apps40 = consumerHelper2.Install(nodes.Get(40));   

  consumerHelper2.SetPrefix("/41");
  consumerHelper2.SetAttribute("MachineRank",StringValue("41"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/41", nodes.Get(41));
  auto apps41 = consumerHelper2.Install(nodes.Get(41));  

  consumerHelper2.SetPrefix("/42");
  consumerHelper2.SetAttribute("MachineRank",StringValue("42"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/42", nodes.Get(42));
  auto apps42 = consumerHelper2.Install(nodes.Get(42));  

  consumerHelper2.SetPrefix("/43");
  consumerHelper2.SetAttribute("MachineRank",StringValue("43"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/43", nodes.Get(43));
  auto apps43 = consumerHelper2.Install(nodes.Get(43));  

  consumerHelper2.SetPrefix("/44");
  consumerHelper2.SetAttribute("MachineRank",StringValue("44"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/44", nodes.Get(44));
  auto apps44 = consumerHelper2.Install(nodes.Get(44));  

  consumerHelper2.SetPrefix("/45");
  consumerHelper2.SetAttribute("MachineRank",StringValue("45"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/45", nodes.Get(45));
  auto apps45 = consumerHelper2.Install(nodes.Get(45));  

  consumerHelper2.SetPrefix("/46");
  consumerHelper2.SetAttribute("MachineRank",StringValue("46"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/46", nodes.Get(46));
  auto apps46 = consumerHelper2.Install(nodes.Get(46));  

  consumerHelper2.SetPrefix("/47");
  consumerHelper2.SetAttribute("MachineRank",StringValue("47"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/47", nodes.Get(47));
  auto apps47 = consumerHelper2.Install(nodes.Get(47));  

  consumerHelper2.SetPrefix("/48");
  consumerHelper2.SetAttribute("MachineRank",StringValue("48"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/48", nodes.Get(48));
  auto apps48 = consumerHelper2.Install(nodes.Get(48));  

  consumerHelper2.SetPrefix("/49");
  consumerHelper2.SetAttribute("MachineRank",StringValue("49"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/49", nodes.Get(49));
  auto apps49 = consumerHelper2.Install(nodes.Get(49));  

  consumerHelper2.SetPrefix("/50");
  consumerHelper2.SetAttribute("MachineRank",StringValue("50"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/50", nodes.Get(50));
  auto apps50 = consumerHelper2.Install(nodes.Get(50));  

  consumerHelper2.SetPrefix("/51");
  consumerHelper2.SetAttribute("MachineRank",StringValue("51"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/51", nodes.Get(51));
  auto apps51 = consumerHelper2.Install(nodes.Get(51));  

  consumerHelper2.SetPrefix("/52");
  consumerHelper2.SetAttribute("MachineRank",StringValue("52"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/52", nodes.Get(52));
  auto apps52 = consumerHelper2.Install(nodes.Get(52));  

  consumerHelper2.SetPrefix("/53");
  consumerHelper2.SetAttribute("MachineRank",StringValue("53"));
  consumerHelper2.SetAttribute("ModelSize",StringValue("3700"));
  consumerHelper2.SetAttribute("TotalMachineNumber",StringValue("54"));
  ndnGlobalRoutingHelper.AddOrigins("/53", nodes.Get(53));
  auto apps53 = consumerHelper2.Install(nodes.Get(53));  

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
  for(int i = 0; i < 100; i++){
    std::cout << "node "<<i<<" send "<< "content size: " << allgatherBytes[i] << std::endl;
  }
  for(int i = 0; i < 100; i++){
    std::cout << "node "<<i<<" send "<< "packet num: " << txBytes[i] << std::endl;
  }
  for(int i = 0; i < 100; i++){
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
