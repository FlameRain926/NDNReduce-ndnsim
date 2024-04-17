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

uint64_t allgatherBytes[313] = {0};
uint64_t txBytes[313] = {0};
uint64_t DropBytes[313] = {0};
uint32_t scattersign[313] = {0};
static int32_t debug_trace = 0;
static int32_t allgather_sign = 0;
static int32_t scatter_sign = 0;
static std::ofstream testFile("234_dragonfly_100M_nocache_timeout.txt");
static std::ofstream test_machine0("234_dragonfly_100M_nocache_send.txt");
static std::ofstream test_machine0_hop("234_dragonfly_100M_nocache_receive.txt");
static std::ofstream test_machine0_cubic("234_dragonfly_100M_nocache_cubicaction.txt");
static double total_hopcount = 0;
static double total_content = 0;
static int64_t packet_sent = 0;

static void
Tracedebug (){
  Time now = Simulator::Now();
  if(scatter_sign != 234){
    std::cout<<"time: "<<now<<"\tscatter packets for 0 : "<<debug_trace<<std::endl;
    std::cout<<"time: "<<now<<"\t0 send packets : "<<packet_sent<<std::endl;
    Simulator::Schedule(Seconds (0.001), &Tracedebug);
    
  }
  else{
    if(allgather_sign != 234)
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
  nodes.Create(312);

  PointToPointHelper p2p;
p2p.Install(nodes.Get(0), nodes.Get(234));
p2p.Install(nodes.Get(1), nodes.Get(234));
p2p.Install(nodes.Get(2), nodes.Get(234));
p2p.Install(nodes.Get(3), nodes.Get(235));
p2p.Install(nodes.Get(4), nodes.Get(235));
p2p.Install(nodes.Get(5), nodes.Get(235));
p2p.Install(nodes.Get(6), nodes.Get(236));
p2p.Install(nodes.Get(7), nodes.Get(236));
p2p.Install(nodes.Get(8), nodes.Get(236));
p2p.Install(nodes.Get(9), nodes.Get(237));
p2p.Install(nodes.Get(10), nodes.Get(237));
p2p.Install(nodes.Get(11), nodes.Get(237));
p2p.Install(nodes.Get(12), nodes.Get(238));
p2p.Install(nodes.Get(13), nodes.Get(238));
p2p.Install(nodes.Get(14), nodes.Get(238));
p2p.Install(nodes.Get(15), nodes.Get(239));
p2p.Install(nodes.Get(16), nodes.Get(239));
p2p.Install(nodes.Get(17), nodes.Get(239));
p2p.Install(nodes.Get(18), nodes.Get(240));
p2p.Install(nodes.Get(19), nodes.Get(240));
p2p.Install(nodes.Get(20), nodes.Get(240));
p2p.Install(nodes.Get(21), nodes.Get(241));
p2p.Install(nodes.Get(22), nodes.Get(241));
p2p.Install(nodes.Get(23), nodes.Get(241));
p2p.Install(nodes.Get(24), nodes.Get(242));
p2p.Install(nodes.Get(25), nodes.Get(242));
p2p.Install(nodes.Get(26), nodes.Get(242));
p2p.Install(nodes.Get(27), nodes.Get(243));
p2p.Install(nodes.Get(28), nodes.Get(243));
p2p.Install(nodes.Get(29), nodes.Get(243));
p2p.Install(nodes.Get(30), nodes.Get(244));
p2p.Install(nodes.Get(31), nodes.Get(244));
p2p.Install(nodes.Get(32), nodes.Get(244));
p2p.Install(nodes.Get(33), nodes.Get(245));
p2p.Install(nodes.Get(34), nodes.Get(245));
p2p.Install(nodes.Get(35), nodes.Get(245));
p2p.Install(nodes.Get(36), nodes.Get(246));
p2p.Install(nodes.Get(37), nodes.Get(246));
p2p.Install(nodes.Get(38), nodes.Get(246));
p2p.Install(nodes.Get(39), nodes.Get(247));
p2p.Install(nodes.Get(40), nodes.Get(247));
p2p.Install(nodes.Get(41), nodes.Get(247));
p2p.Install(nodes.Get(42), nodes.Get(248));
p2p.Install(nodes.Get(43), nodes.Get(248));
p2p.Install(nodes.Get(44), nodes.Get(248));
p2p.Install(nodes.Get(45), nodes.Get(249));
p2p.Install(nodes.Get(46), nodes.Get(249));
p2p.Install(nodes.Get(47), nodes.Get(249));
p2p.Install(nodes.Get(48), nodes.Get(250));
p2p.Install(nodes.Get(49), nodes.Get(250));
p2p.Install(nodes.Get(50), nodes.Get(250));
p2p.Install(nodes.Get(51), nodes.Get(251));
p2p.Install(nodes.Get(52), nodes.Get(251));
p2p.Install(nodes.Get(53), nodes.Get(251));
p2p.Install(nodes.Get(54), nodes.Get(252));
p2p.Install(nodes.Get(55), nodes.Get(252));
p2p.Install(nodes.Get(56), nodes.Get(252));
p2p.Install(nodes.Get(57), nodes.Get(253));
p2p.Install(nodes.Get(58), nodes.Get(253));
p2p.Install(nodes.Get(59), nodes.Get(253));
p2p.Install(nodes.Get(60), nodes.Get(254));
p2p.Install(nodes.Get(61), nodes.Get(254));
p2p.Install(nodes.Get(62), nodes.Get(254));
p2p.Install(nodes.Get(63), nodes.Get(255));
p2p.Install(nodes.Get(64), nodes.Get(255));
p2p.Install(nodes.Get(65), nodes.Get(255));
p2p.Install(nodes.Get(66), nodes.Get(256));
p2p.Install(nodes.Get(67), nodes.Get(256));
p2p.Install(nodes.Get(68), nodes.Get(256));
p2p.Install(nodes.Get(69), nodes.Get(257));
p2p.Install(nodes.Get(70), nodes.Get(257));
p2p.Install(nodes.Get(71), nodes.Get(257));
p2p.Install(nodes.Get(72), nodes.Get(258));
p2p.Install(nodes.Get(73), nodes.Get(258));
p2p.Install(nodes.Get(74), nodes.Get(258));
p2p.Install(nodes.Get(75), nodes.Get(259));
p2p.Install(nodes.Get(76), nodes.Get(259));
p2p.Install(nodes.Get(77), nodes.Get(259));
p2p.Install(nodes.Get(78), nodes.Get(260));
p2p.Install(nodes.Get(79), nodes.Get(260));
p2p.Install(nodes.Get(80), nodes.Get(260));
p2p.Install(nodes.Get(81), nodes.Get(261));
p2p.Install(nodes.Get(82), nodes.Get(261));
p2p.Install(nodes.Get(83), nodes.Get(261));
p2p.Install(nodes.Get(84), nodes.Get(262));
p2p.Install(nodes.Get(85), nodes.Get(262));
p2p.Install(nodes.Get(86), nodes.Get(262));
p2p.Install(nodes.Get(87), nodes.Get(263));
p2p.Install(nodes.Get(88), nodes.Get(263));
p2p.Install(nodes.Get(89), nodes.Get(263));
p2p.Install(nodes.Get(90), nodes.Get(264));
p2p.Install(nodes.Get(91), nodes.Get(264));
p2p.Install(nodes.Get(92), nodes.Get(264));
p2p.Install(nodes.Get(93), nodes.Get(265));
p2p.Install(nodes.Get(94), nodes.Get(265));
p2p.Install(nodes.Get(95), nodes.Get(265));
p2p.Install(nodes.Get(96), nodes.Get(266));
p2p.Install(nodes.Get(97), nodes.Get(266));
p2p.Install(nodes.Get(98), nodes.Get(266));
p2p.Install(nodes.Get(99), nodes.Get(267));
p2p.Install(nodes.Get(100), nodes.Get(267));
p2p.Install(nodes.Get(101), nodes.Get(267));
p2p.Install(nodes.Get(102), nodes.Get(268));
p2p.Install(nodes.Get(103), nodes.Get(268));
p2p.Install(nodes.Get(104), nodes.Get(268));
p2p.Install(nodes.Get(105), nodes.Get(269));
p2p.Install(nodes.Get(106), nodes.Get(269));
p2p.Install(nodes.Get(107), nodes.Get(269));
p2p.Install(nodes.Get(108), nodes.Get(270));
p2p.Install(nodes.Get(109), nodes.Get(270));
p2p.Install(nodes.Get(110), nodes.Get(270));
p2p.Install(nodes.Get(111), nodes.Get(271));
p2p.Install(nodes.Get(112), nodes.Get(271));
p2p.Install(nodes.Get(113), nodes.Get(271));
p2p.Install(nodes.Get(114), nodes.Get(272));
p2p.Install(nodes.Get(115), nodes.Get(272));
p2p.Install(nodes.Get(116), nodes.Get(272));
p2p.Install(nodes.Get(117), nodes.Get(273));
p2p.Install(nodes.Get(118), nodes.Get(273));
p2p.Install(nodes.Get(119), nodes.Get(273));
p2p.Install(nodes.Get(120), nodes.Get(274));
p2p.Install(nodes.Get(121), nodes.Get(274));
p2p.Install(nodes.Get(122), nodes.Get(274));
p2p.Install(nodes.Get(123), nodes.Get(275));
p2p.Install(nodes.Get(124), nodes.Get(275));
p2p.Install(nodes.Get(125), nodes.Get(275));
p2p.Install(nodes.Get(126), nodes.Get(276));
p2p.Install(nodes.Get(127), nodes.Get(276));
p2p.Install(nodes.Get(128), nodes.Get(276));
p2p.Install(nodes.Get(129), nodes.Get(277));
p2p.Install(nodes.Get(130), nodes.Get(277));
p2p.Install(nodes.Get(131), nodes.Get(277));
p2p.Install(nodes.Get(132), nodes.Get(278));
p2p.Install(nodes.Get(133), nodes.Get(278));
p2p.Install(nodes.Get(134), nodes.Get(278));
p2p.Install(nodes.Get(135), nodes.Get(279));
p2p.Install(nodes.Get(136), nodes.Get(279));
p2p.Install(nodes.Get(137), nodes.Get(279));
p2p.Install(nodes.Get(138), nodes.Get(280));
p2p.Install(nodes.Get(139), nodes.Get(280));
p2p.Install(nodes.Get(140), nodes.Get(280));
p2p.Install(nodes.Get(141), nodes.Get(281));
p2p.Install(nodes.Get(142), nodes.Get(281));
p2p.Install(nodes.Get(143), nodes.Get(281));
p2p.Install(nodes.Get(144), nodes.Get(282));
p2p.Install(nodes.Get(145), nodes.Get(282));
p2p.Install(nodes.Get(146), nodes.Get(282));
p2p.Install(nodes.Get(147), nodes.Get(283));
p2p.Install(nodes.Get(148), nodes.Get(283));
p2p.Install(nodes.Get(149), nodes.Get(283));
p2p.Install(nodes.Get(150), nodes.Get(284));
p2p.Install(nodes.Get(151), nodes.Get(284));
p2p.Install(nodes.Get(152), nodes.Get(284));
p2p.Install(nodes.Get(153), nodes.Get(285));
p2p.Install(nodes.Get(154), nodes.Get(285));
p2p.Install(nodes.Get(155), nodes.Get(285));
p2p.Install(nodes.Get(156), nodes.Get(286));
p2p.Install(nodes.Get(157), nodes.Get(286));
p2p.Install(nodes.Get(158), nodes.Get(286));
p2p.Install(nodes.Get(159), nodes.Get(287));
p2p.Install(nodes.Get(160), nodes.Get(287));
p2p.Install(nodes.Get(161), nodes.Get(287));
p2p.Install(nodes.Get(162), nodes.Get(288));
p2p.Install(nodes.Get(163), nodes.Get(288));
p2p.Install(nodes.Get(164), nodes.Get(288));
p2p.Install(nodes.Get(165), nodes.Get(289));
p2p.Install(nodes.Get(166), nodes.Get(289));
p2p.Install(nodes.Get(167), nodes.Get(289));
p2p.Install(nodes.Get(168), nodes.Get(290));
p2p.Install(nodes.Get(169), nodes.Get(290));
p2p.Install(nodes.Get(170), nodes.Get(290));
p2p.Install(nodes.Get(171), nodes.Get(291));
p2p.Install(nodes.Get(172), nodes.Get(291));
p2p.Install(nodes.Get(173), nodes.Get(291));
p2p.Install(nodes.Get(174), nodes.Get(292));
p2p.Install(nodes.Get(175), nodes.Get(292));
p2p.Install(nodes.Get(176), nodes.Get(292));
p2p.Install(nodes.Get(177), nodes.Get(293));
p2p.Install(nodes.Get(178), nodes.Get(293));
p2p.Install(nodes.Get(179), nodes.Get(293));
p2p.Install(nodes.Get(180), nodes.Get(294));
p2p.Install(nodes.Get(181), nodes.Get(294));
p2p.Install(nodes.Get(182), nodes.Get(294));
p2p.Install(nodes.Get(183), nodes.Get(295));
p2p.Install(nodes.Get(184), nodes.Get(295));
p2p.Install(nodes.Get(185), nodes.Get(295));
p2p.Install(nodes.Get(186), nodes.Get(296));
p2p.Install(nodes.Get(187), nodes.Get(296));
p2p.Install(nodes.Get(188), nodes.Get(296));
p2p.Install(nodes.Get(189), nodes.Get(297));
p2p.Install(nodes.Get(190), nodes.Get(297));
p2p.Install(nodes.Get(191), nodes.Get(297));
p2p.Install(nodes.Get(192), nodes.Get(298));
p2p.Install(nodes.Get(193), nodes.Get(298));
p2p.Install(nodes.Get(194), nodes.Get(298));
p2p.Install(nodes.Get(195), nodes.Get(299));
p2p.Install(nodes.Get(196), nodes.Get(299));
p2p.Install(nodes.Get(197), nodes.Get(299));
p2p.Install(nodes.Get(198), nodes.Get(300));
p2p.Install(nodes.Get(199), nodes.Get(300));
p2p.Install(nodes.Get(200), nodes.Get(300));
p2p.Install(nodes.Get(201), nodes.Get(301));
p2p.Install(nodes.Get(202), nodes.Get(301));
p2p.Install(nodes.Get(203), nodes.Get(301));
p2p.Install(nodes.Get(204), nodes.Get(302));
p2p.Install(nodes.Get(205), nodes.Get(302));
p2p.Install(nodes.Get(206), nodes.Get(302));
p2p.Install(nodes.Get(207), nodes.Get(303));
p2p.Install(nodes.Get(208), nodes.Get(303));
p2p.Install(nodes.Get(209), nodes.Get(303));
p2p.Install(nodes.Get(210), nodes.Get(304));
p2p.Install(nodes.Get(211), nodes.Get(304));
p2p.Install(nodes.Get(212), nodes.Get(304));
p2p.Install(nodes.Get(213), nodes.Get(305));
p2p.Install(nodes.Get(214), nodes.Get(305));
p2p.Install(nodes.Get(215), nodes.Get(305));
p2p.Install(nodes.Get(216), nodes.Get(306));
p2p.Install(nodes.Get(217), nodes.Get(306));
p2p.Install(nodes.Get(218), nodes.Get(306));
p2p.Install(nodes.Get(219), nodes.Get(307));
p2p.Install(nodes.Get(220), nodes.Get(307));
p2p.Install(nodes.Get(221), nodes.Get(307));
p2p.Install(nodes.Get(222), nodes.Get(308));
p2p.Install(nodes.Get(223), nodes.Get(308));
p2p.Install(nodes.Get(224), nodes.Get(308));
p2p.Install(nodes.Get(225), nodes.Get(309));
p2p.Install(nodes.Get(226), nodes.Get(309));
p2p.Install(nodes.Get(227), nodes.Get(309));
p2p.Install(nodes.Get(228), nodes.Get(310));
p2p.Install(nodes.Get(229), nodes.Get(310));
p2p.Install(nodes.Get(230), nodes.Get(310));
p2p.Install(nodes.Get(231), nodes.Get(311));
p2p.Install(nodes.Get(232), nodes.Get(311));
p2p.Install(nodes.Get(233), nodes.Get(311));

    // Inner Group

p2p.Install(nodes.Get(234), nodes.Get(235));
p2p.Install(nodes.Get(234), nodes.Get(236));
p2p.Install(nodes.Get(234), nodes.Get(237));
p2p.Install(nodes.Get(234), nodes.Get(238));
p2p.Install(nodes.Get(234), nodes.Get(239));
p2p.Install(nodes.Get(235), nodes.Get(236));
p2p.Install(nodes.Get(235), nodes.Get(237));
p2p.Install(nodes.Get(235), nodes.Get(238));
p2p.Install(nodes.Get(235), nodes.Get(239));
p2p.Install(nodes.Get(236), nodes.Get(237));
p2p.Install(nodes.Get(236), nodes.Get(238));
p2p.Install(nodes.Get(236), nodes.Get(239));
p2p.Install(nodes.Get(237), nodes.Get(238));
p2p.Install(nodes.Get(237), nodes.Get(239));
p2p.Install(nodes.Get(238), nodes.Get(239));
p2p.Install(nodes.Get(240), nodes.Get(241));
p2p.Install(nodes.Get(240), nodes.Get(242));
p2p.Install(nodes.Get(240), nodes.Get(243));
p2p.Install(nodes.Get(240), nodes.Get(244));
p2p.Install(nodes.Get(240), nodes.Get(245));
p2p.Install(nodes.Get(241), nodes.Get(242));
p2p.Install(nodes.Get(241), nodes.Get(243));
p2p.Install(nodes.Get(241), nodes.Get(244));
p2p.Install(nodes.Get(241), nodes.Get(245));
p2p.Install(nodes.Get(242), nodes.Get(243));
p2p.Install(nodes.Get(242), nodes.Get(244));
p2p.Install(nodes.Get(242), nodes.Get(245));
p2p.Install(nodes.Get(243), nodes.Get(244));
p2p.Install(nodes.Get(243), nodes.Get(245));
p2p.Install(nodes.Get(244), nodes.Get(245));
p2p.Install(nodes.Get(246), nodes.Get(247));
p2p.Install(nodes.Get(246), nodes.Get(248));
p2p.Install(nodes.Get(246), nodes.Get(249));
p2p.Install(nodes.Get(246), nodes.Get(250));
p2p.Install(nodes.Get(246), nodes.Get(251));
p2p.Install(nodes.Get(247), nodes.Get(248));
p2p.Install(nodes.Get(247), nodes.Get(249));
p2p.Install(nodes.Get(247), nodes.Get(250));
p2p.Install(nodes.Get(247), nodes.Get(251));
p2p.Install(nodes.Get(248), nodes.Get(249));
p2p.Install(nodes.Get(248), nodes.Get(250));
p2p.Install(nodes.Get(248), nodes.Get(251));
p2p.Install(nodes.Get(249), nodes.Get(250));
p2p.Install(nodes.Get(249), nodes.Get(251));
p2p.Install(nodes.Get(250), nodes.Get(251));
p2p.Install(nodes.Get(252), nodes.Get(253));
p2p.Install(nodes.Get(252), nodes.Get(254));
p2p.Install(nodes.Get(252), nodes.Get(255));
p2p.Install(nodes.Get(252), nodes.Get(256));
p2p.Install(nodes.Get(252), nodes.Get(257));
p2p.Install(nodes.Get(253), nodes.Get(254));
p2p.Install(nodes.Get(253), nodes.Get(255));
p2p.Install(nodes.Get(253), nodes.Get(256));
p2p.Install(nodes.Get(253), nodes.Get(257));
p2p.Install(nodes.Get(254), nodes.Get(255));
p2p.Install(nodes.Get(254), nodes.Get(256));
p2p.Install(nodes.Get(254), nodes.Get(257));
p2p.Install(nodes.Get(255), nodes.Get(256));
p2p.Install(nodes.Get(255), nodes.Get(257));
p2p.Install(nodes.Get(256), nodes.Get(257));
p2p.Install(nodes.Get(258), nodes.Get(259));
p2p.Install(nodes.Get(258), nodes.Get(260));
p2p.Install(nodes.Get(258), nodes.Get(261));
p2p.Install(nodes.Get(258), nodes.Get(262));
p2p.Install(nodes.Get(258), nodes.Get(263));
p2p.Install(nodes.Get(259), nodes.Get(260));
p2p.Install(nodes.Get(259), nodes.Get(261));
p2p.Install(nodes.Get(259), nodes.Get(262));
p2p.Install(nodes.Get(259), nodes.Get(263));
p2p.Install(nodes.Get(260), nodes.Get(261));
p2p.Install(nodes.Get(260), nodes.Get(262));
p2p.Install(nodes.Get(260), nodes.Get(263));
p2p.Install(nodes.Get(261), nodes.Get(262));
p2p.Install(nodes.Get(261), nodes.Get(263));
p2p.Install(nodes.Get(262), nodes.Get(263));
p2p.Install(nodes.Get(264), nodes.Get(265));
p2p.Install(nodes.Get(264), nodes.Get(266));
p2p.Install(nodes.Get(264), nodes.Get(267));
p2p.Install(nodes.Get(264), nodes.Get(268));
p2p.Install(nodes.Get(264), nodes.Get(269));
p2p.Install(nodes.Get(265), nodes.Get(266));
p2p.Install(nodes.Get(265), nodes.Get(267));
p2p.Install(nodes.Get(265), nodes.Get(268));
p2p.Install(nodes.Get(265), nodes.Get(269));
p2p.Install(nodes.Get(266), nodes.Get(267));
p2p.Install(nodes.Get(266), nodes.Get(268));
p2p.Install(nodes.Get(266), nodes.Get(269));
p2p.Install(nodes.Get(267), nodes.Get(268));
p2p.Install(nodes.Get(267), nodes.Get(269));
p2p.Install(nodes.Get(268), nodes.Get(269));
p2p.Install(nodes.Get(270), nodes.Get(271));
p2p.Install(nodes.Get(270), nodes.Get(272));
p2p.Install(nodes.Get(270), nodes.Get(273));
p2p.Install(nodes.Get(270), nodes.Get(274));
p2p.Install(nodes.Get(270), nodes.Get(275));
p2p.Install(nodes.Get(271), nodes.Get(272));
p2p.Install(nodes.Get(271), nodes.Get(273));
p2p.Install(nodes.Get(271), nodes.Get(274));
p2p.Install(nodes.Get(271), nodes.Get(275));
p2p.Install(nodes.Get(272), nodes.Get(273));
p2p.Install(nodes.Get(272), nodes.Get(274));
p2p.Install(nodes.Get(272), nodes.Get(275));
p2p.Install(nodes.Get(273), nodes.Get(274));
p2p.Install(nodes.Get(273), nodes.Get(275));
p2p.Install(nodes.Get(274), nodes.Get(275));
p2p.Install(nodes.Get(276), nodes.Get(277));
p2p.Install(nodes.Get(276), nodes.Get(278));
p2p.Install(nodes.Get(276), nodes.Get(279));
p2p.Install(nodes.Get(276), nodes.Get(280));
p2p.Install(nodes.Get(276), nodes.Get(281));
p2p.Install(nodes.Get(277), nodes.Get(278));
p2p.Install(nodes.Get(277), nodes.Get(279));
p2p.Install(nodes.Get(277), nodes.Get(280));
p2p.Install(nodes.Get(277), nodes.Get(281));
p2p.Install(nodes.Get(278), nodes.Get(279));
p2p.Install(nodes.Get(278), nodes.Get(280));
p2p.Install(nodes.Get(278), nodes.Get(281));
p2p.Install(nodes.Get(279), nodes.Get(280));
p2p.Install(nodes.Get(279), nodes.Get(281));
p2p.Install(nodes.Get(280), nodes.Get(281));
p2p.Install(nodes.Get(282), nodes.Get(283));
p2p.Install(nodes.Get(282), nodes.Get(284));
p2p.Install(nodes.Get(282), nodes.Get(285));
p2p.Install(nodes.Get(282), nodes.Get(286));
p2p.Install(nodes.Get(282), nodes.Get(287));
p2p.Install(nodes.Get(283), nodes.Get(284));
p2p.Install(nodes.Get(283), nodes.Get(285));
p2p.Install(nodes.Get(283), nodes.Get(286));
p2p.Install(nodes.Get(283), nodes.Get(287));
p2p.Install(nodes.Get(284), nodes.Get(285));
p2p.Install(nodes.Get(284), nodes.Get(286));
p2p.Install(nodes.Get(284), nodes.Get(287));
p2p.Install(nodes.Get(285), nodes.Get(286));
p2p.Install(nodes.Get(285), nodes.Get(287));
p2p.Install(nodes.Get(286), nodes.Get(287));
p2p.Install(nodes.Get(288), nodes.Get(289));
p2p.Install(nodes.Get(288), nodes.Get(290));
p2p.Install(nodes.Get(288), nodes.Get(291));
p2p.Install(nodes.Get(288), nodes.Get(292));
p2p.Install(nodes.Get(288), nodes.Get(293));
p2p.Install(nodes.Get(289), nodes.Get(290));
p2p.Install(nodes.Get(289), nodes.Get(291));
p2p.Install(nodes.Get(289), nodes.Get(292));
p2p.Install(nodes.Get(289), nodes.Get(293));
p2p.Install(nodes.Get(290), nodes.Get(291));
p2p.Install(nodes.Get(290), nodes.Get(292));
p2p.Install(nodes.Get(290), nodes.Get(293));
p2p.Install(nodes.Get(291), nodes.Get(292));
p2p.Install(nodes.Get(291), nodes.Get(293));
p2p.Install(nodes.Get(292), nodes.Get(293));
p2p.Install(nodes.Get(294), nodes.Get(295));
p2p.Install(nodes.Get(294), nodes.Get(296));
p2p.Install(nodes.Get(294), nodes.Get(297));
p2p.Install(nodes.Get(294), nodes.Get(298));
p2p.Install(nodes.Get(294), nodes.Get(299));
p2p.Install(nodes.Get(295), nodes.Get(296));
p2p.Install(nodes.Get(295), nodes.Get(297));
p2p.Install(nodes.Get(295), nodes.Get(298));
p2p.Install(nodes.Get(295), nodes.Get(299));
p2p.Install(nodes.Get(296), nodes.Get(297));
p2p.Install(nodes.Get(296), nodes.Get(298));
p2p.Install(nodes.Get(296), nodes.Get(299));
p2p.Install(nodes.Get(297), nodes.Get(298));
p2p.Install(nodes.Get(297), nodes.Get(299));
p2p.Install(nodes.Get(298), nodes.Get(299));
p2p.Install(nodes.Get(300), nodes.Get(301));
p2p.Install(nodes.Get(300), nodes.Get(302));
p2p.Install(nodes.Get(300), nodes.Get(303));
p2p.Install(nodes.Get(300), nodes.Get(304));
p2p.Install(nodes.Get(300), nodes.Get(305));
p2p.Install(nodes.Get(301), nodes.Get(302));
p2p.Install(nodes.Get(301), nodes.Get(303));
p2p.Install(nodes.Get(301), nodes.Get(304));
p2p.Install(nodes.Get(301), nodes.Get(305));
p2p.Install(nodes.Get(302), nodes.Get(303));
p2p.Install(nodes.Get(302), nodes.Get(304));
p2p.Install(nodes.Get(302), nodes.Get(305));
p2p.Install(nodes.Get(303), nodes.Get(304));
p2p.Install(nodes.Get(303), nodes.Get(305));
p2p.Install(nodes.Get(304), nodes.Get(305));
p2p.Install(nodes.Get(306), nodes.Get(307));
p2p.Install(nodes.Get(306), nodes.Get(308));
p2p.Install(nodes.Get(306), nodes.Get(309));
p2p.Install(nodes.Get(306), nodes.Get(310));
p2p.Install(nodes.Get(306), nodes.Get(311));
p2p.Install(nodes.Get(307), nodes.Get(308));
p2p.Install(nodes.Get(307), nodes.Get(309));
p2p.Install(nodes.Get(307), nodes.Get(310));
p2p.Install(nodes.Get(307), nodes.Get(311));
p2p.Install(nodes.Get(308), nodes.Get(309));
p2p.Install(nodes.Get(308), nodes.Get(310));
p2p.Install(nodes.Get(308), nodes.Get(311));
p2p.Install(nodes.Get(309), nodes.Get(310));
p2p.Install(nodes.Get(309), nodes.Get(311));
p2p.Install(nodes.Get(310), nodes.Get(311));

    // Inter Group

p2p.Install(nodes.Get(311), nodes.Get(234));
p2p.Install(nodes.Get(311), nodes.Get(240));
p2p.Install(nodes.Get(310), nodes.Get(247));
p2p.Install(nodes.Get(310), nodes.Get(253));
p2p.Install(nodes.Get(309), nodes.Get(260));
p2p.Install(nodes.Get(309), nodes.Get(266));
p2p.Install(nodes.Get(308), nodes.Get(273));
p2p.Install(nodes.Get(308), nodes.Get(279));
p2p.Install(nodes.Get(307), nodes.Get(286));
p2p.Install(nodes.Get(307), nodes.Get(292));
p2p.Install(nodes.Get(306), nodes.Get(299));
p2p.Install(nodes.Get(306), nodes.Get(305));
p2p.Install(nodes.Get(305), nodes.Get(234));
p2p.Install(nodes.Get(304), nodes.Get(241));
p2p.Install(nodes.Get(304), nodes.Get(247));
p2p.Install(nodes.Get(303), nodes.Get(254));
p2p.Install(nodes.Get(303), nodes.Get(260));
p2p.Install(nodes.Get(302), nodes.Get(267));
p2p.Install(nodes.Get(302), nodes.Get(273));
p2p.Install(nodes.Get(301), nodes.Get(280));
p2p.Install(nodes.Get(301), nodes.Get(286));
p2p.Install(nodes.Get(300), nodes.Get(293));
p2p.Install(nodes.Get(300), nodes.Get(299));
p2p.Install(nodes.Get(298), nodes.Get(235));
p2p.Install(nodes.Get(298), nodes.Get(241));
p2p.Install(nodes.Get(297), nodes.Get(248));
p2p.Install(nodes.Get(297), nodes.Get(254));
p2p.Install(nodes.Get(296), nodes.Get(261));
p2p.Install(nodes.Get(296), nodes.Get(267));
p2p.Install(nodes.Get(295), nodes.Get(274));
p2p.Install(nodes.Get(295), nodes.Get(280));
p2p.Install(nodes.Get(294), nodes.Get(287));
p2p.Install(nodes.Get(294), nodes.Get(293));
p2p.Install(nodes.Get(292), nodes.Get(235));
p2p.Install(nodes.Get(291), nodes.Get(242));
p2p.Install(nodes.Get(291), nodes.Get(248));
p2p.Install(nodes.Get(290), nodes.Get(255));
p2p.Install(nodes.Get(290), nodes.Get(261));
p2p.Install(nodes.Get(289), nodes.Get(268));
p2p.Install(nodes.Get(289), nodes.Get(274));
p2p.Install(nodes.Get(288), nodes.Get(281));
p2p.Install(nodes.Get(288), nodes.Get(287));
p2p.Install(nodes.Get(285), nodes.Get(236));
p2p.Install(nodes.Get(285), nodes.Get(242));
p2p.Install(nodes.Get(284), nodes.Get(249));
p2p.Install(nodes.Get(284), nodes.Get(255));
p2p.Install(nodes.Get(283), nodes.Get(262));
p2p.Install(nodes.Get(283), nodes.Get(268));
p2p.Install(nodes.Get(282), nodes.Get(275));
p2p.Install(nodes.Get(282), nodes.Get(281));
p2p.Install(nodes.Get(279), nodes.Get(236));
p2p.Install(nodes.Get(278), nodes.Get(243));
p2p.Install(nodes.Get(278), nodes.Get(249));
p2p.Install(nodes.Get(277), nodes.Get(256));
p2p.Install(nodes.Get(277), nodes.Get(262));
p2p.Install(nodes.Get(276), nodes.Get(269));
p2p.Install(nodes.Get(276), nodes.Get(275));
p2p.Install(nodes.Get(272), nodes.Get(237));
p2p.Install(nodes.Get(272), nodes.Get(243));
p2p.Install(nodes.Get(271), nodes.Get(250));
p2p.Install(nodes.Get(271), nodes.Get(256));
p2p.Install(nodes.Get(270), nodes.Get(263));
p2p.Install(nodes.Get(270), nodes.Get(269));
p2p.Install(nodes.Get(266), nodes.Get(237));
p2p.Install(nodes.Get(265), nodes.Get(244));
p2p.Install(nodes.Get(265), nodes.Get(250));
p2p.Install(nodes.Get(264), nodes.Get(257));
p2p.Install(nodes.Get(264), nodes.Get(263));
p2p.Install(nodes.Get(259), nodes.Get(238));
p2p.Install(nodes.Get(259), nodes.Get(244));
p2p.Install(nodes.Get(258), nodes.Get(251));
p2p.Install(nodes.Get(258), nodes.Get(257));
p2p.Install(nodes.Get(253), nodes.Get(238));
p2p.Install(nodes.Get(252), nodes.Get(245));
p2p.Install(nodes.Get(252), nodes.Get(251));
p2p.Install(nodes.Get(246), nodes.Get(239));
p2p.Install(nodes.Get(246), nodes.Get(245));
p2p.Install(nodes.Get(240), nodes.Get(239));


  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(0);
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
  // ndn::StrategyChoiceHelper::Install(nodes.Get(2),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(3),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(12),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(13),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(22),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(23),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(32),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(33),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(7),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(17),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(6),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(7),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(16),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(17),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(26),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(27),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(36),"/","/localhost/nfd/strategy/random");
  // ndn::StrategyChoiceHelper::Install(nodes.Get(37),"/","/localhost/nfd/strategy/random");
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
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/0", nodes.Get(0));
    auto apps0= consumerHelper.Install(nodes.Get(0)); 

    consumerHelper.SetPrefix("/1");
    consumerHelper.SetAttribute("MachineRank",StringValue("1"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/1", nodes.Get(1));
    auto apps1= consumerHelper.Install(nodes.Get(1)); 

    consumerHelper.SetPrefix("/2");
    consumerHelper.SetAttribute("MachineRank",StringValue("2"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/2", nodes.Get(2));
    auto apps2= consumerHelper.Install(nodes.Get(2)); 

    consumerHelper.SetPrefix("/3");
    consumerHelper.SetAttribute("MachineRank",StringValue("3"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/3", nodes.Get(3));
    auto apps3= consumerHelper.Install(nodes.Get(3)); 

    consumerHelper.SetPrefix("/4");
    consumerHelper.SetAttribute("MachineRank",StringValue("4"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/4", nodes.Get(4));
    auto apps4= consumerHelper.Install(nodes.Get(4)); 

    consumerHelper.SetPrefix("/5");
    consumerHelper.SetAttribute("MachineRank",StringValue("5"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/5", nodes.Get(5));
    auto apps5= consumerHelper.Install(nodes.Get(5)); 

    consumerHelper.SetPrefix("/6");
    consumerHelper.SetAttribute("MachineRank",StringValue("6"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/6", nodes.Get(6));
    auto apps6= consumerHelper.Install(nodes.Get(6)); 

    consumerHelper.SetPrefix("/7");
    consumerHelper.SetAttribute("MachineRank",StringValue("7"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/7", nodes.Get(7));
    auto apps7= consumerHelper.Install(nodes.Get(7)); 

    consumerHelper.SetPrefix("/8");
    consumerHelper.SetAttribute("MachineRank",StringValue("8"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/8", nodes.Get(8));
    auto apps8= consumerHelper.Install(nodes.Get(8)); 

    consumerHelper.SetPrefix("/9");
    consumerHelper.SetAttribute("MachineRank",StringValue("9"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/9", nodes.Get(9));
    auto apps9= consumerHelper.Install(nodes.Get(9)); 

    consumerHelper.SetPrefix("/10");
    consumerHelper.SetAttribute("MachineRank",StringValue("10"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/10", nodes.Get(10));
    auto apps10= consumerHelper.Install(nodes.Get(10)); 

    consumerHelper.SetPrefix("/11");
    consumerHelper.SetAttribute("MachineRank",StringValue("11"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/11", nodes.Get(11));
    auto apps11= consumerHelper.Install(nodes.Get(11)); 

    consumerHelper.SetPrefix("/12");
    consumerHelper.SetAttribute("MachineRank",StringValue("12"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/12", nodes.Get(12));
    auto apps12= consumerHelper.Install(nodes.Get(12)); 

    consumerHelper.SetPrefix("/13");
    consumerHelper.SetAttribute("MachineRank",StringValue("13"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/13", nodes.Get(13));
    auto apps13= consumerHelper.Install(nodes.Get(13)); 

    consumerHelper.SetPrefix("/14");
    consumerHelper.SetAttribute("MachineRank",StringValue("14"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/14", nodes.Get(14));
    auto apps14= consumerHelper.Install(nodes.Get(14)); 

    consumerHelper.SetPrefix("/15");
    consumerHelper.SetAttribute("MachineRank",StringValue("15"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/15", nodes.Get(15));
    auto apps15= consumerHelper.Install(nodes.Get(15)); 

    consumerHelper.SetPrefix("/16");
    consumerHelper.SetAttribute("MachineRank",StringValue("16"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/16", nodes.Get(16));
    auto apps16= consumerHelper.Install(nodes.Get(16)); 

    consumerHelper.SetPrefix("/17");
    consumerHelper.SetAttribute("MachineRank",StringValue("17"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/17", nodes.Get(17));
    auto apps17= consumerHelper.Install(nodes.Get(17)); 

    consumerHelper.SetPrefix("/18");
    consumerHelper.SetAttribute("MachineRank",StringValue("18"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/18", nodes.Get(18));
    auto apps18= consumerHelper.Install(nodes.Get(18)); 

    consumerHelper.SetPrefix("/19");
    consumerHelper.SetAttribute("MachineRank",StringValue("19"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/19", nodes.Get(19));
    auto apps19= consumerHelper.Install(nodes.Get(19)); 

    consumerHelper.SetPrefix("/20");
    consumerHelper.SetAttribute("MachineRank",StringValue("20"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/20", nodes.Get(20));
    auto apps20= consumerHelper.Install(nodes.Get(20)); 

    consumerHelper.SetPrefix("/21");
    consumerHelper.SetAttribute("MachineRank",StringValue("21"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/21", nodes.Get(21));
    auto apps21= consumerHelper.Install(nodes.Get(21)); 

    consumerHelper.SetPrefix("/22");
    consumerHelper.SetAttribute("MachineRank",StringValue("22"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/22", nodes.Get(22));
    auto apps22= consumerHelper.Install(nodes.Get(22)); 

    consumerHelper.SetPrefix("/23");
    consumerHelper.SetAttribute("MachineRank",StringValue("23"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/23", nodes.Get(23));
    auto apps23= consumerHelper.Install(nodes.Get(23)); 

    consumerHelper.SetPrefix("/24");
    consumerHelper.SetAttribute("MachineRank",StringValue("24"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/24", nodes.Get(24));
    auto apps24= consumerHelper.Install(nodes.Get(24)); 

    consumerHelper.SetPrefix("/25");
    consumerHelper.SetAttribute("MachineRank",StringValue("25"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/25", nodes.Get(25));
    auto apps25= consumerHelper.Install(nodes.Get(25)); 

    consumerHelper.SetPrefix("/26");
    consumerHelper.SetAttribute("MachineRank",StringValue("26"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/26", nodes.Get(26));
    auto apps26= consumerHelper.Install(nodes.Get(26)); 

    consumerHelper.SetPrefix("/27");
    consumerHelper.SetAttribute("MachineRank",StringValue("27"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/27", nodes.Get(27));
    auto apps27= consumerHelper.Install(nodes.Get(27)); 

    consumerHelper.SetPrefix("/28");
    consumerHelper.SetAttribute("MachineRank",StringValue("28"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/28", nodes.Get(28));
    auto apps28= consumerHelper.Install(nodes.Get(28)); 

    consumerHelper.SetPrefix("/29");
    consumerHelper.SetAttribute("MachineRank",StringValue("29"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/29", nodes.Get(29));
    auto apps29= consumerHelper.Install(nodes.Get(29)); 

    consumerHelper.SetPrefix("/30");
    consumerHelper.SetAttribute("MachineRank",StringValue("30"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/30", nodes.Get(30));
    auto apps30= consumerHelper.Install(nodes.Get(30)); 

    consumerHelper.SetPrefix("/31");
    consumerHelper.SetAttribute("MachineRank",StringValue("31"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/31", nodes.Get(31));
    auto apps31= consumerHelper.Install(nodes.Get(31)); 

    consumerHelper.SetPrefix("/32");
    consumerHelper.SetAttribute("MachineRank",StringValue("32"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/32", nodes.Get(32));
    auto apps32= consumerHelper.Install(nodes.Get(32)); 

    consumerHelper.SetPrefix("/33");
    consumerHelper.SetAttribute("MachineRank",StringValue("33"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/33", nodes.Get(33));
    auto apps33= consumerHelper.Install(nodes.Get(33)); 

    consumerHelper.SetPrefix("/34");
    consumerHelper.SetAttribute("MachineRank",StringValue("34"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/34", nodes.Get(34));
    auto apps34= consumerHelper.Install(nodes.Get(34)); 

    consumerHelper.SetPrefix("/35");
    consumerHelper.SetAttribute("MachineRank",StringValue("35"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/35", nodes.Get(35));
    auto apps35= consumerHelper.Install(nodes.Get(35)); 

    consumerHelper.SetPrefix("/36");
    consumerHelper.SetAttribute("MachineRank",StringValue("36"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/36", nodes.Get(36));
    auto apps36= consumerHelper.Install(nodes.Get(36)); 

    consumerHelper.SetPrefix("/37");
    consumerHelper.SetAttribute("MachineRank",StringValue("37"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/37", nodes.Get(37));
    auto apps37= consumerHelper.Install(nodes.Get(37)); 

    consumerHelper.SetPrefix("/38");
    consumerHelper.SetAttribute("MachineRank",StringValue("38"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/38", nodes.Get(38));
    auto apps38= consumerHelper.Install(nodes.Get(38)); 

    consumerHelper.SetPrefix("/39");
    consumerHelper.SetAttribute("MachineRank",StringValue("39"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/39", nodes.Get(39));
    auto apps39= consumerHelper.Install(nodes.Get(39)); 

    consumerHelper.SetPrefix("/40");
    consumerHelper.SetAttribute("MachineRank",StringValue("40"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/40", nodes.Get(40));
    auto apps40= consumerHelper.Install(nodes.Get(40)); 

    consumerHelper.SetPrefix("/41");
    consumerHelper.SetAttribute("MachineRank",StringValue("41"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/41", nodes.Get(41));
    auto apps41= consumerHelper.Install(nodes.Get(41)); 

    consumerHelper.SetPrefix("/42");
    consumerHelper.SetAttribute("MachineRank",StringValue("42"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/42", nodes.Get(42));
    auto apps42= consumerHelper.Install(nodes.Get(42)); 

    consumerHelper.SetPrefix("/43");
    consumerHelper.SetAttribute("MachineRank",StringValue("43"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/43", nodes.Get(43));
    auto apps43= consumerHelper.Install(nodes.Get(43)); 

    consumerHelper.SetPrefix("/44");
    consumerHelper.SetAttribute("MachineRank",StringValue("44"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/44", nodes.Get(44));
    auto apps44= consumerHelper.Install(nodes.Get(44)); 

    consumerHelper.SetPrefix("/45");
    consumerHelper.SetAttribute("MachineRank",StringValue("45"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/45", nodes.Get(45));
    auto apps45= consumerHelper.Install(nodes.Get(45)); 

    consumerHelper.SetPrefix("/46");
    consumerHelper.SetAttribute("MachineRank",StringValue("46"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/46", nodes.Get(46));
    auto apps46= consumerHelper.Install(nodes.Get(46)); 

    consumerHelper.SetPrefix("/47");
    consumerHelper.SetAttribute("MachineRank",StringValue("47"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/47", nodes.Get(47));
    auto apps47= consumerHelper.Install(nodes.Get(47)); 

    consumerHelper.SetPrefix("/48");
    consumerHelper.SetAttribute("MachineRank",StringValue("48"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/48", nodes.Get(48));
    auto apps48= consumerHelper.Install(nodes.Get(48)); 

    consumerHelper.SetPrefix("/49");
    consumerHelper.SetAttribute("MachineRank",StringValue("49"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/49", nodes.Get(49));
    auto apps49= consumerHelper.Install(nodes.Get(49)); 

    consumerHelper.SetPrefix("/50");
    consumerHelper.SetAttribute("MachineRank",StringValue("50"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/50", nodes.Get(50));
    auto apps50= consumerHelper.Install(nodes.Get(50)); 

    consumerHelper.SetPrefix("/51");
    consumerHelper.SetAttribute("MachineRank",StringValue("51"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/51", nodes.Get(51));
    auto apps51= consumerHelper.Install(nodes.Get(51)); 

    consumerHelper.SetPrefix("/52");
    consumerHelper.SetAttribute("MachineRank",StringValue("52"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/52", nodes.Get(52));
    auto apps52= consumerHelper.Install(nodes.Get(52)); 

    consumerHelper.SetPrefix("/53");
    consumerHelper.SetAttribute("MachineRank",StringValue("53"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/53", nodes.Get(53));
    auto apps53= consumerHelper.Install(nodes.Get(53)); 

    consumerHelper.SetPrefix("/54");
    consumerHelper.SetAttribute("MachineRank",StringValue("54"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/54", nodes.Get(54));
    auto apps54= consumerHelper.Install(nodes.Get(54)); 

    consumerHelper.SetPrefix("/55");
    consumerHelper.SetAttribute("MachineRank",StringValue("55"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/55", nodes.Get(55));
    auto apps55= consumerHelper.Install(nodes.Get(55)); 

    consumerHelper.SetPrefix("/56");
    consumerHelper.SetAttribute("MachineRank",StringValue("56"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/56", nodes.Get(56));
    auto apps56= consumerHelper.Install(nodes.Get(56)); 

    consumerHelper.SetPrefix("/57");
    consumerHelper.SetAttribute("MachineRank",StringValue("57"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/57", nodes.Get(57));
    auto apps57= consumerHelper.Install(nodes.Get(57)); 

    consumerHelper.SetPrefix("/58");
    consumerHelper.SetAttribute("MachineRank",StringValue("58"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/58", nodes.Get(58));
    auto apps58= consumerHelper.Install(nodes.Get(58)); 

    consumerHelper.SetPrefix("/59");
    consumerHelper.SetAttribute("MachineRank",StringValue("59"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/59", nodes.Get(59));
    auto apps59= consumerHelper.Install(nodes.Get(59)); 

    consumerHelper.SetPrefix("/60");
    consumerHelper.SetAttribute("MachineRank",StringValue("60"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/60", nodes.Get(60));
    auto apps60= consumerHelper.Install(nodes.Get(60)); 

    consumerHelper.SetPrefix("/61");
    consumerHelper.SetAttribute("MachineRank",StringValue("61"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/61", nodes.Get(61));
    auto apps61= consumerHelper.Install(nodes.Get(61)); 

    consumerHelper.SetPrefix("/62");
    consumerHelper.SetAttribute("MachineRank",StringValue("62"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/62", nodes.Get(62));
    auto apps62= consumerHelper.Install(nodes.Get(62)); 

    consumerHelper.SetPrefix("/63");
    consumerHelper.SetAttribute("MachineRank",StringValue("63"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/63", nodes.Get(63));
    auto apps63= consumerHelper.Install(nodes.Get(63)); 

    consumerHelper.SetPrefix("/64");
    consumerHelper.SetAttribute("MachineRank",StringValue("64"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/64", nodes.Get(64));
    auto apps64= consumerHelper.Install(nodes.Get(64)); 

    consumerHelper.SetPrefix("/65");
    consumerHelper.SetAttribute("MachineRank",StringValue("65"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/65", nodes.Get(65));
    auto apps65= consumerHelper.Install(nodes.Get(65)); 

    consumerHelper.SetPrefix("/66");
    consumerHelper.SetAttribute("MachineRank",StringValue("66"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/66", nodes.Get(66));
    auto apps66= consumerHelper.Install(nodes.Get(66)); 

    consumerHelper.SetPrefix("/67");
    consumerHelper.SetAttribute("MachineRank",StringValue("67"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/67", nodes.Get(67));
    auto apps67= consumerHelper.Install(nodes.Get(67)); 

    consumerHelper.SetPrefix("/68");
    consumerHelper.SetAttribute("MachineRank",StringValue("68"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/68", nodes.Get(68));
    auto apps68= consumerHelper.Install(nodes.Get(68)); 

    consumerHelper.SetPrefix("/69");
    consumerHelper.SetAttribute("MachineRank",StringValue("69"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/69", nodes.Get(69));
    auto apps69= consumerHelper.Install(nodes.Get(69)); 

    consumerHelper.SetPrefix("/70");
    consumerHelper.SetAttribute("MachineRank",StringValue("70"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/70", nodes.Get(70));
    auto apps70= consumerHelper.Install(nodes.Get(70)); 

    consumerHelper.SetPrefix("/71");
    consumerHelper.SetAttribute("MachineRank",StringValue("71"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/71", nodes.Get(71));
    auto apps71= consumerHelper.Install(nodes.Get(71)); 

    consumerHelper.SetPrefix("/72");
    consumerHelper.SetAttribute("MachineRank",StringValue("72"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/72", nodes.Get(72));
    auto apps72= consumerHelper.Install(nodes.Get(72)); 

    consumerHelper.SetPrefix("/73");
    consumerHelper.SetAttribute("MachineRank",StringValue("73"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/73", nodes.Get(73));
    auto apps73= consumerHelper.Install(nodes.Get(73)); 

    consumerHelper.SetPrefix("/74");
    consumerHelper.SetAttribute("MachineRank",StringValue("74"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/74", nodes.Get(74));
    auto apps74= consumerHelper.Install(nodes.Get(74)); 

    consumerHelper.SetPrefix("/75");
    consumerHelper.SetAttribute("MachineRank",StringValue("75"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/75", nodes.Get(75));
    auto apps75= consumerHelper.Install(nodes.Get(75)); 

    consumerHelper.SetPrefix("/76");
    consumerHelper.SetAttribute("MachineRank",StringValue("76"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/76", nodes.Get(76));
    auto apps76= consumerHelper.Install(nodes.Get(76)); 

    consumerHelper.SetPrefix("/77");
    consumerHelper.SetAttribute("MachineRank",StringValue("77"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/77", nodes.Get(77));
    auto apps77= consumerHelper.Install(nodes.Get(77)); 

    consumerHelper.SetPrefix("/78");
    consumerHelper.SetAttribute("MachineRank",StringValue("78"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/78", nodes.Get(78));
    auto apps78= consumerHelper.Install(nodes.Get(78)); 

    consumerHelper.SetPrefix("/79");
    consumerHelper.SetAttribute("MachineRank",StringValue("79"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/79", nodes.Get(79));
    auto apps79= consumerHelper.Install(nodes.Get(79)); 

    consumerHelper.SetPrefix("/80");
    consumerHelper.SetAttribute("MachineRank",StringValue("80"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/80", nodes.Get(80));
    auto apps80= consumerHelper.Install(nodes.Get(80)); 

    consumerHelper.SetPrefix("/81");
    consumerHelper.SetAttribute("MachineRank",StringValue("81"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/81", nodes.Get(81));
    auto apps81= consumerHelper.Install(nodes.Get(81)); 

    consumerHelper.SetPrefix("/82");
    consumerHelper.SetAttribute("MachineRank",StringValue("82"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/82", nodes.Get(82));
    auto apps82= consumerHelper.Install(nodes.Get(82)); 

    consumerHelper.SetPrefix("/83");
    consumerHelper.SetAttribute("MachineRank",StringValue("83"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/83", nodes.Get(83));
    auto apps83= consumerHelper.Install(nodes.Get(83)); 

    consumerHelper.SetPrefix("/84");
    consumerHelper.SetAttribute("MachineRank",StringValue("84"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/84", nodes.Get(84));
    auto apps84= consumerHelper.Install(nodes.Get(84)); 

    consumerHelper.SetPrefix("/85");
    consumerHelper.SetAttribute("MachineRank",StringValue("85"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/85", nodes.Get(85));
    auto apps85= consumerHelper.Install(nodes.Get(85)); 

    consumerHelper.SetPrefix("/86");
    consumerHelper.SetAttribute("MachineRank",StringValue("86"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/86", nodes.Get(86));
    auto apps86= consumerHelper.Install(nodes.Get(86)); 

    consumerHelper.SetPrefix("/87");
    consumerHelper.SetAttribute("MachineRank",StringValue("87"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/87", nodes.Get(87));
    auto apps87= consumerHelper.Install(nodes.Get(87)); 

    consumerHelper.SetPrefix("/88");
    consumerHelper.SetAttribute("MachineRank",StringValue("88"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/88", nodes.Get(88));
    auto apps88= consumerHelper.Install(nodes.Get(88)); 

    consumerHelper.SetPrefix("/89");
    consumerHelper.SetAttribute("MachineRank",StringValue("89"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/89", nodes.Get(89));
    auto apps89= consumerHelper.Install(nodes.Get(89)); 

    consumerHelper.SetPrefix("/90");
    consumerHelper.SetAttribute("MachineRank",StringValue("90"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/90", nodes.Get(90));
    auto apps90= consumerHelper.Install(nodes.Get(90)); 

    consumerHelper.SetPrefix("/91");
    consumerHelper.SetAttribute("MachineRank",StringValue("91"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/91", nodes.Get(91));
    auto apps91= consumerHelper.Install(nodes.Get(91)); 

    consumerHelper.SetPrefix("/92");
    consumerHelper.SetAttribute("MachineRank",StringValue("92"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/92", nodes.Get(92));
    auto apps92= consumerHelper.Install(nodes.Get(92)); 

    consumerHelper.SetPrefix("/93");
    consumerHelper.SetAttribute("MachineRank",StringValue("93"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/93", nodes.Get(93));
    auto apps93= consumerHelper.Install(nodes.Get(93)); 

    consumerHelper.SetPrefix("/94");
    consumerHelper.SetAttribute("MachineRank",StringValue("94"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/94", nodes.Get(94));
    auto apps94= consumerHelper.Install(nodes.Get(94)); 

    consumerHelper.SetPrefix("/95");
    consumerHelper.SetAttribute("MachineRank",StringValue("95"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/95", nodes.Get(95));
    auto apps95= consumerHelper.Install(nodes.Get(95)); 

    consumerHelper.SetPrefix("/96");
    consumerHelper.SetAttribute("MachineRank",StringValue("96"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/96", nodes.Get(96));
    auto apps96= consumerHelper.Install(nodes.Get(96)); 

    consumerHelper.SetPrefix("/97");
    consumerHelper.SetAttribute("MachineRank",StringValue("97"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/97", nodes.Get(97));
    auto apps97= consumerHelper.Install(nodes.Get(97)); 

    consumerHelper.SetPrefix("/98");
    consumerHelper.SetAttribute("MachineRank",StringValue("98"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/98", nodes.Get(98));
    auto apps98= consumerHelper.Install(nodes.Get(98)); 

    consumerHelper.SetPrefix("/99");
    consumerHelper.SetAttribute("MachineRank",StringValue("99"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/99", nodes.Get(99));
    auto apps99= consumerHelper.Install(nodes.Get(99)); 

    consumerHelper.SetPrefix("/100");
    consumerHelper.SetAttribute("MachineRank",StringValue("100"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/100", nodes.Get(100));
    auto apps100= consumerHelper.Install(nodes.Get(100)); 

    consumerHelper.SetPrefix("/101");
    consumerHelper.SetAttribute("MachineRank",StringValue("101"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/101", nodes.Get(101));
    auto apps101= consumerHelper.Install(nodes.Get(101)); 

    consumerHelper.SetPrefix("/102");
    consumerHelper.SetAttribute("MachineRank",StringValue("102"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/102", nodes.Get(102));
    auto apps102= consumerHelper.Install(nodes.Get(102)); 

    consumerHelper.SetPrefix("/103");
    consumerHelper.SetAttribute("MachineRank",StringValue("103"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/103", nodes.Get(103));
    auto apps103= consumerHelper.Install(nodes.Get(103)); 

    consumerHelper.SetPrefix("/104");
    consumerHelper.SetAttribute("MachineRank",StringValue("104"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/104", nodes.Get(104));
    auto apps104= consumerHelper.Install(nodes.Get(104)); 

    consumerHelper.SetPrefix("/105");
    consumerHelper.SetAttribute("MachineRank",StringValue("105"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/105", nodes.Get(105));
    auto apps105= consumerHelper.Install(nodes.Get(105)); 

    consumerHelper.SetPrefix("/106");
    consumerHelper.SetAttribute("MachineRank",StringValue("106"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/106", nodes.Get(106));
    auto apps106= consumerHelper.Install(nodes.Get(106)); 

    consumerHelper.SetPrefix("/107");
    consumerHelper.SetAttribute("MachineRank",StringValue("107"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/107", nodes.Get(107));
    auto apps107= consumerHelper.Install(nodes.Get(107)); 

    consumerHelper.SetPrefix("/108");
    consumerHelper.SetAttribute("MachineRank",StringValue("108"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/108", nodes.Get(108));
    auto apps108= consumerHelper.Install(nodes.Get(108)); 

    consumerHelper.SetPrefix("/109");
    consumerHelper.SetAttribute("MachineRank",StringValue("109"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/109", nodes.Get(109));
    auto apps109= consumerHelper.Install(nodes.Get(109)); 

    consumerHelper.SetPrefix("/110");
    consumerHelper.SetAttribute("MachineRank",StringValue("110"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/110", nodes.Get(110));
    auto apps110= consumerHelper.Install(nodes.Get(110)); 

    consumerHelper.SetPrefix("/111");
    consumerHelper.SetAttribute("MachineRank",StringValue("111"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/111", nodes.Get(111));
    auto apps111= consumerHelper.Install(nodes.Get(111)); 

    consumerHelper.SetPrefix("/112");
    consumerHelper.SetAttribute("MachineRank",StringValue("112"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/112", nodes.Get(112));
    auto apps112= consumerHelper.Install(nodes.Get(112)); 

    consumerHelper.SetPrefix("/113");
    consumerHelper.SetAttribute("MachineRank",StringValue("113"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/113", nodes.Get(113));
    auto apps113= consumerHelper.Install(nodes.Get(113)); 

    consumerHelper.SetPrefix("/114");
    consumerHelper.SetAttribute("MachineRank",StringValue("114"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/114", nodes.Get(114));
    auto apps114= consumerHelper.Install(nodes.Get(114)); 

    consumerHelper.SetPrefix("/115");
    consumerHelper.SetAttribute("MachineRank",StringValue("115"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/115", nodes.Get(115));
    auto apps115= consumerHelper.Install(nodes.Get(115)); 

    consumerHelper.SetPrefix("/116");
    consumerHelper.SetAttribute("MachineRank",StringValue("116"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/116", nodes.Get(116));
    auto apps116= consumerHelper.Install(nodes.Get(116)); 

    consumerHelper.SetPrefix("/117");
    consumerHelper.SetAttribute("MachineRank",StringValue("117"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/117", nodes.Get(117));
    auto apps117= consumerHelper.Install(nodes.Get(117)); 

    consumerHelper.SetPrefix("/118");
    consumerHelper.SetAttribute("MachineRank",StringValue("118"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/118", nodes.Get(118));
    auto apps118= consumerHelper.Install(nodes.Get(118)); 

    consumerHelper.SetPrefix("/119");
    consumerHelper.SetAttribute("MachineRank",StringValue("119"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/119", nodes.Get(119));
    auto apps119= consumerHelper.Install(nodes.Get(119)); 

    consumerHelper.SetPrefix("/120");
    consumerHelper.SetAttribute("MachineRank",StringValue("120"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/120", nodes.Get(120));
    auto apps120= consumerHelper.Install(nodes.Get(120)); 

    consumerHelper.SetPrefix("/121");
    consumerHelper.SetAttribute("MachineRank",StringValue("121"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/121", nodes.Get(121));
    auto apps121= consumerHelper.Install(nodes.Get(121)); 

    consumerHelper.SetPrefix("/122");
    consumerHelper.SetAttribute("MachineRank",StringValue("122"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/122", nodes.Get(122));
    auto apps122= consumerHelper.Install(nodes.Get(122)); 

    consumerHelper.SetPrefix("/123");
    consumerHelper.SetAttribute("MachineRank",StringValue("123"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/123", nodes.Get(123));
    auto apps123= consumerHelper.Install(nodes.Get(123)); 

    consumerHelper.SetPrefix("/124");
    consumerHelper.SetAttribute("MachineRank",StringValue("124"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/124", nodes.Get(124));
    auto apps124= consumerHelper.Install(nodes.Get(124)); 

    consumerHelper.SetPrefix("/125");
    consumerHelper.SetAttribute("MachineRank",StringValue("125"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/125", nodes.Get(125));
    auto apps125= consumerHelper.Install(nodes.Get(125)); 

    consumerHelper.SetPrefix("/126");
    consumerHelper.SetAttribute("MachineRank",StringValue("126"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/126", nodes.Get(126));
    auto apps126= consumerHelper.Install(nodes.Get(126)); 

    consumerHelper.SetPrefix("/127");
    consumerHelper.SetAttribute("MachineRank",StringValue("127"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/127", nodes.Get(127));
    auto apps127= consumerHelper.Install(nodes.Get(127)); 

    consumerHelper.SetPrefix("/128");
    consumerHelper.SetAttribute("MachineRank",StringValue("128"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/128", nodes.Get(128));
    auto apps128= consumerHelper.Install(nodes.Get(128)); 

    consumerHelper.SetPrefix("/129");
    consumerHelper.SetAttribute("MachineRank",StringValue("129"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/129", nodes.Get(129));
    auto apps129= consumerHelper.Install(nodes.Get(129)); 

    consumerHelper.SetPrefix("/130");
    consumerHelper.SetAttribute("MachineRank",StringValue("130"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/130", nodes.Get(130));
    auto apps130= consumerHelper.Install(nodes.Get(130)); 

    consumerHelper.SetPrefix("/131");
    consumerHelper.SetAttribute("MachineRank",StringValue("131"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/131", nodes.Get(131));
    auto apps131= consumerHelper.Install(nodes.Get(131)); 

    consumerHelper.SetPrefix("/132");
    consumerHelper.SetAttribute("MachineRank",StringValue("132"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/132", nodes.Get(132));
    auto apps132= consumerHelper.Install(nodes.Get(132)); 

    consumerHelper.SetPrefix("/133");
    consumerHelper.SetAttribute("MachineRank",StringValue("133"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/133", nodes.Get(133));
    auto apps133= consumerHelper.Install(nodes.Get(133)); 

    consumerHelper.SetPrefix("/134");
    consumerHelper.SetAttribute("MachineRank",StringValue("134"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/134", nodes.Get(134));
    auto apps134= consumerHelper.Install(nodes.Get(134)); 

    consumerHelper.SetPrefix("/135");
    consumerHelper.SetAttribute("MachineRank",StringValue("135"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/135", nodes.Get(135));
    auto apps135= consumerHelper.Install(nodes.Get(135)); 

    consumerHelper.SetPrefix("/136");
    consumerHelper.SetAttribute("MachineRank",StringValue("136"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/136", nodes.Get(136));
    auto apps136= consumerHelper.Install(nodes.Get(136)); 

    consumerHelper.SetPrefix("/137");
    consumerHelper.SetAttribute("MachineRank",StringValue("137"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/137", nodes.Get(137));
    auto apps137= consumerHelper.Install(nodes.Get(137)); 

    consumerHelper.SetPrefix("/138");
    consumerHelper.SetAttribute("MachineRank",StringValue("138"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/138", nodes.Get(138));
    auto apps138= consumerHelper.Install(nodes.Get(138)); 

    consumerHelper.SetPrefix("/139");
    consumerHelper.SetAttribute("MachineRank",StringValue("139"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/139", nodes.Get(139));
    auto apps139= consumerHelper.Install(nodes.Get(139)); 

    consumerHelper.SetPrefix("/140");
    consumerHelper.SetAttribute("MachineRank",StringValue("140"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/140", nodes.Get(140));
    auto apps140= consumerHelper.Install(nodes.Get(140)); 

    consumerHelper.SetPrefix("/141");
    consumerHelper.SetAttribute("MachineRank",StringValue("141"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/141", nodes.Get(141));
    auto apps141= consumerHelper.Install(nodes.Get(141)); 

    consumerHelper.SetPrefix("/142");
    consumerHelper.SetAttribute("MachineRank",StringValue("142"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/142", nodes.Get(142));
    auto apps142= consumerHelper.Install(nodes.Get(142)); 

    consumerHelper.SetPrefix("/143");
    consumerHelper.SetAttribute("MachineRank",StringValue("143"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/143", nodes.Get(143));
    auto apps143= consumerHelper.Install(nodes.Get(143)); 

    consumerHelper.SetPrefix("/144");
    consumerHelper.SetAttribute("MachineRank",StringValue("144"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/144", nodes.Get(144));
    auto apps144= consumerHelper.Install(nodes.Get(144)); 

    consumerHelper.SetPrefix("/145");
    consumerHelper.SetAttribute("MachineRank",StringValue("145"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/145", nodes.Get(145));
    auto apps145= consumerHelper.Install(nodes.Get(145)); 

    consumerHelper.SetPrefix("/146");
    consumerHelper.SetAttribute("MachineRank",StringValue("146"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/146", nodes.Get(146));
    auto apps146= consumerHelper.Install(nodes.Get(146)); 

    consumerHelper.SetPrefix("/147");
    consumerHelper.SetAttribute("MachineRank",StringValue("147"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/147", nodes.Get(147));
    auto apps147= consumerHelper.Install(nodes.Get(147)); 

    consumerHelper.SetPrefix("/148");
    consumerHelper.SetAttribute("MachineRank",StringValue("148"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/148", nodes.Get(148));
    auto apps148= consumerHelper.Install(nodes.Get(148)); 

    consumerHelper.SetPrefix("/149");
    consumerHelper.SetAttribute("MachineRank",StringValue("149"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/149", nodes.Get(149));
    auto apps149= consumerHelper.Install(nodes.Get(149)); 

    consumerHelper.SetPrefix("/150");
    consumerHelper.SetAttribute("MachineRank",StringValue("150"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/150", nodes.Get(150));
    auto apps150= consumerHelper.Install(nodes.Get(150)); 

    consumerHelper.SetPrefix("/151");
    consumerHelper.SetAttribute("MachineRank",StringValue("151"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/151", nodes.Get(151));
    auto apps151= consumerHelper.Install(nodes.Get(151)); 

    consumerHelper.SetPrefix("/152");
    consumerHelper.SetAttribute("MachineRank",StringValue("152"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/152", nodes.Get(152));
    auto apps152= consumerHelper.Install(nodes.Get(152)); 

    consumerHelper.SetPrefix("/153");
    consumerHelper.SetAttribute("MachineRank",StringValue("153"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/153", nodes.Get(153));
    auto apps153= consumerHelper.Install(nodes.Get(153)); 

    consumerHelper.SetPrefix("/154");
    consumerHelper.SetAttribute("MachineRank",StringValue("154"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/154", nodes.Get(154));
    auto apps154= consumerHelper.Install(nodes.Get(154)); 

    consumerHelper.SetPrefix("/155");
    consumerHelper.SetAttribute("MachineRank",StringValue("155"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/155", nodes.Get(155));
    auto apps155= consumerHelper.Install(nodes.Get(155)); 

    consumerHelper.SetPrefix("/156");
    consumerHelper.SetAttribute("MachineRank",StringValue("156"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/156", nodes.Get(156));
    auto apps156= consumerHelper.Install(nodes.Get(156)); 

    consumerHelper.SetPrefix("/157");
    consumerHelper.SetAttribute("MachineRank",StringValue("157"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/157", nodes.Get(157));
    auto apps157= consumerHelper.Install(nodes.Get(157)); 

    consumerHelper.SetPrefix("/158");
    consumerHelper.SetAttribute("MachineRank",StringValue("158"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/158", nodes.Get(158));
    auto apps158= consumerHelper.Install(nodes.Get(158)); 

    consumerHelper.SetPrefix("/159");
    consumerHelper.SetAttribute("MachineRank",StringValue("159"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/159", nodes.Get(159));
    auto apps159= consumerHelper.Install(nodes.Get(159)); 

    consumerHelper.SetPrefix("/160");
    consumerHelper.SetAttribute("MachineRank",StringValue("160"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/160", nodes.Get(160));
    auto apps160= consumerHelper.Install(nodes.Get(160)); 

    consumerHelper.SetPrefix("/161");
    consumerHelper.SetAttribute("MachineRank",StringValue("161"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/161", nodes.Get(161));
    auto apps161= consumerHelper.Install(nodes.Get(161)); 

    consumerHelper.SetPrefix("/162");
    consumerHelper.SetAttribute("MachineRank",StringValue("162"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/162", nodes.Get(162));
    auto apps162= consumerHelper.Install(nodes.Get(162)); 

    consumerHelper.SetPrefix("/163");
    consumerHelper.SetAttribute("MachineRank",StringValue("163"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/163", nodes.Get(163));
    auto apps163= consumerHelper.Install(nodes.Get(163)); 

    consumerHelper.SetPrefix("/164");
    consumerHelper.SetAttribute("MachineRank",StringValue("164"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/164", nodes.Get(164));
    auto apps164= consumerHelper.Install(nodes.Get(164)); 

    consumerHelper.SetPrefix("/165");
    consumerHelper.SetAttribute("MachineRank",StringValue("165"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/165", nodes.Get(165));
    auto apps165= consumerHelper.Install(nodes.Get(165)); 

    consumerHelper.SetPrefix("/166");
    consumerHelper.SetAttribute("MachineRank",StringValue("166"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/166", nodes.Get(166));
    auto apps166= consumerHelper.Install(nodes.Get(166)); 

    consumerHelper.SetPrefix("/167");
    consumerHelper.SetAttribute("MachineRank",StringValue("167"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/167", nodes.Get(167));
    auto apps167= consumerHelper.Install(nodes.Get(167)); 

    consumerHelper.SetPrefix("/168");
    consumerHelper.SetAttribute("MachineRank",StringValue("168"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/168", nodes.Get(168));
    auto apps168= consumerHelper.Install(nodes.Get(168)); 

    consumerHelper.SetPrefix("/169");
    consumerHelper.SetAttribute("MachineRank",StringValue("169"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/169", nodes.Get(169));
    auto apps169= consumerHelper.Install(nodes.Get(169)); 

    consumerHelper.SetPrefix("/170");
    consumerHelper.SetAttribute("MachineRank",StringValue("170"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/170", nodes.Get(170));
    auto apps170= consumerHelper.Install(nodes.Get(170)); 

    consumerHelper.SetPrefix("/171");
    consumerHelper.SetAttribute("MachineRank",StringValue("171"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/171", nodes.Get(171));
    auto apps171= consumerHelper.Install(nodes.Get(171)); 

    consumerHelper.SetPrefix("/172");
    consumerHelper.SetAttribute("MachineRank",StringValue("172"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/172", nodes.Get(172));
    auto apps172= consumerHelper.Install(nodes.Get(172)); 

    consumerHelper.SetPrefix("/173");
    consumerHelper.SetAttribute("MachineRank",StringValue("173"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/173", nodes.Get(173));
    auto apps173= consumerHelper.Install(nodes.Get(173)); 

    consumerHelper.SetPrefix("/174");
    consumerHelper.SetAttribute("MachineRank",StringValue("174"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/174", nodes.Get(174));
    auto apps174= consumerHelper.Install(nodes.Get(174)); 

    consumerHelper.SetPrefix("/175");
    consumerHelper.SetAttribute("MachineRank",StringValue("175"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/175", nodes.Get(175));
    auto apps175= consumerHelper.Install(nodes.Get(175)); 

    consumerHelper.SetPrefix("/176");
    consumerHelper.SetAttribute("MachineRank",StringValue("176"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/176", nodes.Get(176));
    auto apps176= consumerHelper.Install(nodes.Get(176)); 

    consumerHelper.SetPrefix("/177");
    consumerHelper.SetAttribute("MachineRank",StringValue("177"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/177", nodes.Get(177));
    auto apps177= consumerHelper.Install(nodes.Get(177)); 

    consumerHelper.SetPrefix("/178");
    consumerHelper.SetAttribute("MachineRank",StringValue("178"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/178", nodes.Get(178));
    auto apps178= consumerHelper.Install(nodes.Get(178)); 

    consumerHelper.SetPrefix("/179");
    consumerHelper.SetAttribute("MachineRank",StringValue("179"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/179", nodes.Get(179));
    auto apps179= consumerHelper.Install(nodes.Get(179)); 

    consumerHelper.SetPrefix("/180");
    consumerHelper.SetAttribute("MachineRank",StringValue("180"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/180", nodes.Get(180));
    auto apps180= consumerHelper.Install(nodes.Get(180)); 

    consumerHelper.SetPrefix("/181");
    consumerHelper.SetAttribute("MachineRank",StringValue("181"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/181", nodes.Get(181));
    auto apps181= consumerHelper.Install(nodes.Get(181)); 

    consumerHelper.SetPrefix("/182");
    consumerHelper.SetAttribute("MachineRank",StringValue("182"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/182", nodes.Get(182));
    auto apps182= consumerHelper.Install(nodes.Get(182)); 

    consumerHelper.SetPrefix("/183");
    consumerHelper.SetAttribute("MachineRank",StringValue("183"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/183", nodes.Get(183));
    auto apps183= consumerHelper.Install(nodes.Get(183)); 

    consumerHelper.SetPrefix("/184");
    consumerHelper.SetAttribute("MachineRank",StringValue("184"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/184", nodes.Get(184));
    auto apps184= consumerHelper.Install(nodes.Get(184)); 

    consumerHelper.SetPrefix("/185");
    consumerHelper.SetAttribute("MachineRank",StringValue("185"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/185", nodes.Get(185));
    auto apps185= consumerHelper.Install(nodes.Get(185)); 

    consumerHelper.SetPrefix("/186");
    consumerHelper.SetAttribute("MachineRank",StringValue("186"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/186", nodes.Get(186));
    auto apps186= consumerHelper.Install(nodes.Get(186)); 

    consumerHelper.SetPrefix("/187");
    consumerHelper.SetAttribute("MachineRank",StringValue("187"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/187", nodes.Get(187));
    auto apps187= consumerHelper.Install(nodes.Get(187)); 

    consumerHelper.SetPrefix("/188");
    consumerHelper.SetAttribute("MachineRank",StringValue("188"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/188", nodes.Get(188));
    auto apps188= consumerHelper.Install(nodes.Get(188)); 

    consumerHelper.SetPrefix("/189");
    consumerHelper.SetAttribute("MachineRank",StringValue("189"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/189", nodes.Get(189));
    auto apps189= consumerHelper.Install(nodes.Get(189)); 

    consumerHelper.SetPrefix("/190");
    consumerHelper.SetAttribute("MachineRank",StringValue("190"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/190", nodes.Get(190));
    auto apps190= consumerHelper.Install(nodes.Get(190)); 

    consumerHelper.SetPrefix("/191");
    consumerHelper.SetAttribute("MachineRank",StringValue("191"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/191", nodes.Get(191));
    auto apps191= consumerHelper.Install(nodes.Get(191)); 

    consumerHelper.SetPrefix("/192");
    consumerHelper.SetAttribute("MachineRank",StringValue("192"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/192", nodes.Get(192));
    auto apps192= consumerHelper.Install(nodes.Get(192)); 

    consumerHelper.SetPrefix("/193");
    consumerHelper.SetAttribute("MachineRank",StringValue("193"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/193", nodes.Get(193));
    auto apps193= consumerHelper.Install(nodes.Get(193)); 

    consumerHelper.SetPrefix("/194");
    consumerHelper.SetAttribute("MachineRank",StringValue("194"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/194", nodes.Get(194));
    auto apps194= consumerHelper.Install(nodes.Get(194)); 

    consumerHelper.SetPrefix("/195");
    consumerHelper.SetAttribute("MachineRank",StringValue("195"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/195", nodes.Get(195));
    auto apps195= consumerHelper.Install(nodes.Get(195)); 

    consumerHelper.SetPrefix("/196");
    consumerHelper.SetAttribute("MachineRank",StringValue("196"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/196", nodes.Get(196));
    auto apps196= consumerHelper.Install(nodes.Get(196)); 

    consumerHelper.SetPrefix("/197");
    consumerHelper.SetAttribute("MachineRank",StringValue("197"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/197", nodes.Get(197));
    auto apps197= consumerHelper.Install(nodes.Get(197)); 

    consumerHelper.SetPrefix("/198");
    consumerHelper.SetAttribute("MachineRank",StringValue("198"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/198", nodes.Get(198));
    auto apps198= consumerHelper.Install(nodes.Get(198)); 

    consumerHelper.SetPrefix("/199");
    consumerHelper.SetAttribute("MachineRank",StringValue("199"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/199", nodes.Get(199));
    auto apps199= consumerHelper.Install(nodes.Get(199)); 

    consumerHelper.SetPrefix("/200");
    consumerHelper.SetAttribute("MachineRank",StringValue("200"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/200", nodes.Get(200));
    auto apps200= consumerHelper.Install(nodes.Get(200)); 

    consumerHelper.SetPrefix("/201");
    consumerHelper.SetAttribute("MachineRank",StringValue("201"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/201", nodes.Get(201));
    auto apps201= consumerHelper.Install(nodes.Get(201)); 

    consumerHelper.SetPrefix("/202");
    consumerHelper.SetAttribute("MachineRank",StringValue("202"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/202", nodes.Get(202));
    auto apps202= consumerHelper.Install(nodes.Get(202)); 

    consumerHelper.SetPrefix("/203");
    consumerHelper.SetAttribute("MachineRank",StringValue("203"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/203", nodes.Get(203));
    auto apps203= consumerHelper.Install(nodes.Get(203)); 

    consumerHelper.SetPrefix("/204");
    consumerHelper.SetAttribute("MachineRank",StringValue("204"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/204", nodes.Get(204));
    auto apps204= consumerHelper.Install(nodes.Get(204)); 

    consumerHelper.SetPrefix("/205");
    consumerHelper.SetAttribute("MachineRank",StringValue("205"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/205", nodes.Get(205));
    auto apps205= consumerHelper.Install(nodes.Get(205)); 

    consumerHelper.SetPrefix("/206");
    consumerHelper.SetAttribute("MachineRank",StringValue("206"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/206", nodes.Get(206));
    auto apps206= consumerHelper.Install(nodes.Get(206)); 

    consumerHelper.SetPrefix("/207");
    consumerHelper.SetAttribute("MachineRank",StringValue("207"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/207", nodes.Get(207));
    auto apps207= consumerHelper.Install(nodes.Get(207)); 

    consumerHelper.SetPrefix("/208");
    consumerHelper.SetAttribute("MachineRank",StringValue("208"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/208", nodes.Get(208));
    auto apps208= consumerHelper.Install(nodes.Get(208)); 

    consumerHelper.SetPrefix("/209");
    consumerHelper.SetAttribute("MachineRank",StringValue("209"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/209", nodes.Get(209));
    auto apps209= consumerHelper.Install(nodes.Get(209)); 

    consumerHelper.SetPrefix("/210");
    consumerHelper.SetAttribute("MachineRank",StringValue("210"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/210", nodes.Get(210));
    auto apps210= consumerHelper.Install(nodes.Get(210)); 

    consumerHelper.SetPrefix("/211");
    consumerHelper.SetAttribute("MachineRank",StringValue("211"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/211", nodes.Get(211));
    auto apps211= consumerHelper.Install(nodes.Get(211)); 

    consumerHelper.SetPrefix("/212");
    consumerHelper.SetAttribute("MachineRank",StringValue("212"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/212", nodes.Get(212));
    auto apps212= consumerHelper.Install(nodes.Get(212)); 

    consumerHelper.SetPrefix("/213");
    consumerHelper.SetAttribute("MachineRank",StringValue("213"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/213", nodes.Get(213));
    auto apps213= consumerHelper.Install(nodes.Get(213)); 

    consumerHelper.SetPrefix("/214");
    consumerHelper.SetAttribute("MachineRank",StringValue("214"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/214", nodes.Get(214));
    auto apps214= consumerHelper.Install(nodes.Get(214)); 

    consumerHelper.SetPrefix("/215");
    consumerHelper.SetAttribute("MachineRank",StringValue("215"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/215", nodes.Get(215));
    auto apps215= consumerHelper.Install(nodes.Get(215)); 

    consumerHelper.SetPrefix("/216");
    consumerHelper.SetAttribute("MachineRank",StringValue("216"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/216", nodes.Get(216));
    auto apps216= consumerHelper.Install(nodes.Get(216)); 

    consumerHelper.SetPrefix("/217");
    consumerHelper.SetAttribute("MachineRank",StringValue("217"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/217", nodes.Get(217));
    auto apps217= consumerHelper.Install(nodes.Get(217)); 

    consumerHelper.SetPrefix("/218");
    consumerHelper.SetAttribute("MachineRank",StringValue("218"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/218", nodes.Get(218));
    auto apps218= consumerHelper.Install(nodes.Get(218)); 

    consumerHelper.SetPrefix("/219");
    consumerHelper.SetAttribute("MachineRank",StringValue("219"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/219", nodes.Get(219));
    auto apps219= consumerHelper.Install(nodes.Get(219)); 

    consumerHelper.SetPrefix("/220");
    consumerHelper.SetAttribute("MachineRank",StringValue("220"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/220", nodes.Get(220));
    auto apps220= consumerHelper.Install(nodes.Get(220)); 

    consumerHelper.SetPrefix("/221");
    consumerHelper.SetAttribute("MachineRank",StringValue("221"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/221", nodes.Get(221));
    auto apps221= consumerHelper.Install(nodes.Get(221)); 

    consumerHelper.SetPrefix("/222");
    consumerHelper.SetAttribute("MachineRank",StringValue("222"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/222", nodes.Get(222));
    auto apps222= consumerHelper.Install(nodes.Get(222)); 

    consumerHelper.SetPrefix("/223");
    consumerHelper.SetAttribute("MachineRank",StringValue("223"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/223", nodes.Get(223));
    auto apps223= consumerHelper.Install(nodes.Get(223)); 

    consumerHelper.SetPrefix("/224");
    consumerHelper.SetAttribute("MachineRank",StringValue("224"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/224", nodes.Get(224));
    auto apps224= consumerHelper.Install(nodes.Get(224)); 

    consumerHelper.SetPrefix("/225");
    consumerHelper.SetAttribute("MachineRank",StringValue("225"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/225", nodes.Get(225));
    auto apps225= consumerHelper.Install(nodes.Get(225)); 

    consumerHelper.SetPrefix("/226");
    consumerHelper.SetAttribute("MachineRank",StringValue("226"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/226", nodes.Get(226));
    auto apps226= consumerHelper.Install(nodes.Get(226)); 

    consumerHelper.SetPrefix("/227");
    consumerHelper.SetAttribute("MachineRank",StringValue("227"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/227", nodes.Get(227));
    auto apps227= consumerHelper.Install(nodes.Get(227)); 

    consumerHelper.SetPrefix("/228");
    consumerHelper.SetAttribute("MachineRank",StringValue("228"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/228", nodes.Get(228));
    auto apps228= consumerHelper.Install(nodes.Get(228)); 

    consumerHelper.SetPrefix("/229");
    consumerHelper.SetAttribute("MachineRank",StringValue("229"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/229", nodes.Get(229));
    auto apps229= consumerHelper.Install(nodes.Get(229)); 

    consumerHelper.SetPrefix("/230");
    consumerHelper.SetAttribute("MachineRank",StringValue("230"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/230", nodes.Get(230));
    auto apps230= consumerHelper.Install(nodes.Get(230)); 

    consumerHelper.SetPrefix("/231");
    consumerHelper.SetAttribute("MachineRank",StringValue("231"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/231", nodes.Get(231));
    auto apps231= consumerHelper.Install(nodes.Get(231)); 

    consumerHelper.SetPrefix("/232");
    consumerHelper.SetAttribute("MachineRank",StringValue("232"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/232", nodes.Get(232));
    auto apps232= consumerHelper.Install(nodes.Get(232)); 

    consumerHelper.SetPrefix("/233");
    consumerHelper.SetAttribute("MachineRank",StringValue("233"));
    consumerHelper.SetAttribute("ModelSize",StringValue("400"));
    consumerHelper.SetAttribute("TotalMachineNumber",StringValue("234"));
    ndnGlobalRoutingHelper.AddOrigins("/233", nodes.Get(233));
    auto apps233= consumerHelper.Install(nodes.Get(233));

  // u_int16_t ndnml::scatfin_machine = 0;
  //u_int16_t ndnml::all_scatter_fin = 0;
  //Calculate and install FIBs
  //GlobalRoutingHelper::CalculateLfidRoutes();
  GlobalRoutingHelper::CalculateRoutes();

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
  for(int i = 0; i < 313; i++){
    std::cout << "node "<<i<<" send "<< "content size: " << allgatherBytes[i] << std::endl;
  }
  for(int i = 0; i < 313; i++){
    std::cout << "node "<<i<<" send "<< "packet num: " << txBytes[i] << std::endl;
  }
  for(int i = 0; i < 313; i++){
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
