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

#include "ndn-ml.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-stack-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.ndnml");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ndnml);

constexpr double ndnml::CUBIC_C;

TypeId
ndnml::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ndnml")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<ndnml>()
      .AddAttribute("StartSeq", "Initial sequence number", IntegerValue(0),
                    MakeIntegerAccessor(&ndnml::m_seq), MakeIntegerChecker<int32_t>())

      .AddAttribute("Prefix_interest", "Name of the Interest", StringValue("/"),
                    MakeNameAccessor(&ndnml::m_interestName), MakeNameChecker())
      .AddAttribute("LifeTime", "LifeTime for interest packet", StringValue("2s"),
                    MakeTimeAccessor(&ndnml::m_interestLifeTime), MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),
                    MakeTimeAccessor(&ndnml::GetRetxTimer, &ndnml::SetRetxTimer),
                    MakeTimeChecker())

      .AddTraceSource("LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor(&ndnml::m_lastRetransmittedInterestDataDelay),
                      "ns3::ndn::Consumer::LastRetransmittedInterestDataDelayCallback")

      .AddTraceSource("FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor(&ndnml::m_firstInterestDataDelay),
                      "ns3::ndn::Consumer::FirstInterestDataDelayCallback")
      .AddTraceSource("Allgathersize",
                      "Trace the size of allgather data a machine send in total",
                      MakeTraceSourceAccessor(&ndnml::m_allgathersize),
                      "ns3::ndn::Consumer::FirstInterestDataDelayCallback")

      .AddAttribute("MachineRank", "Machine Rank", StringValue("0"),
                    MakeIntegerAccessor(&ndnml::m_rank), MakeIntegerChecker<int>())
      .AddAttribute("TotalMachineNumber", "Frequency of interest packets", StringValue("1"),
                    MakeIntegerAccessor(&ndnml::total_machine), MakeIntegerChecker<int>())
      .AddAttribute("ModelSize", "Model size", StringValue("1"),
                    MakeIntegerAccessor(&ndnml::model_size), MakeIntegerChecker<int>())

      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&ndnml::m_prefix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1020),
                    MakeUintegerAccessor(&ndnml::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&ndnml::m_keyLocator), MakeNameChecker());

  return tid;
}

ndnml::ndnml()
  : m_rand(CreateObject<UniformRandomVariable>())
  , m_seq(0)
  //lxd
  , total_machine(5),
    scatter_flag(true),
    m_frequency(1136.0),
    m_firstTime(true)
    //ndn-consumer-window
    , m_initialWindow(1)
    , m_setInitialWindowOnTimeout(true)
    , m_window(1)
    , m_inFlight(0)
    //ndn-consumer-pcon
    , m_reactToCongestionMarks(true)
    , m_addRttSuppress(0.5)
    , m_useCwa(true)
    , m_useCubicFastConv(true)
    , m_cubicBeta(0.8)
    , m_ssthresh(std::numeric_limits<double>::max())
    , m_highData(0)
    , m_recPoint(0.0)
    , m_cubicWmax(0)
    , m_cubicLastWmax(0)
    , m_cubicLastDecrease(time::steady_clock::now())
{
  NS_LOG_FUNCTION_NOARGS();
  m_seqMax = std::numeric_limits<uint32_t>::max();

  m_rtt = CreateObject<RttMeanDeviation>();
  //need to gap a model size of sequence in allgather phaze
  m_seqgapbase = (model_size+1)*(total_machine-1)+m_rank*model_size;
}

void
ndnml::SetRetxTimer(Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_retxEvent); // slower, but better for memory
  }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule(m_retxTimer, &ndnml::CheckRetxTimeout, this);
}

Time
ndnml::GetRetxTimer() const
{
  return m_retxTimer;
}

void
ndnml::CheckRetxTimeout()
{
  Time now = Simulator::Now();
  Time rto = m_rtt->RetransmitTimeout();
  // NS_LOG_DEBUG ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  while (!m_seqTimeouts.empty()) {
    SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
      m_seqTimeouts.get<i_timestamp>().begin();
    if (entry->time + rto <= now) // timeout expired? 
    {
      uint32_t seqNo = entry->seq;
      m_seqTimeouts.get<i_timestamp>().erase(entry);
      OnTimeout(seqNo);
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &ndnml::CheckRetxTimeout, this);
}

// Application Methods
void
ndnml::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  // do base stuff
  App::StartApplication();
  
  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
  //std::cout<<"point"<<GetNode()->GetId()<<std::endl;
  //ScheduleNextPacket();
  train();
  
}

void
ndnml::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
}

void
ndnml::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    seq = m_seq++;
  }

  //
  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);
  nameWithSequence->append("test");
  //

  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  interest->setCanBePrefix(false);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << seq);

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  std::cout<<"enter wrong"<<GetNode()->GetId()<<std::endl;
  //ScheduleNextPacket();
}

void
ndnml::SendPacket1(const int flag,const int prefix_to_send,const int data_sequence)
{
  if (!m_active){
    std::cout<<"no active"<<std::endl;
    return;}

  NS_LOG_FUNCTION_NOARGS();

  bool retx_flag = false;
  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid
  //std::cout<<"retrans size: "<<m_retxSeqs.size()<<std::endl;
  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    std::cout<<"enter retrans"<<seq<<std::endl;
    m_retxSeqs.erase(m_retxSeqs.begin());
    retx_flag = true;
    break;
  }


  if (seq == std::numeric_limits<uint32_t>::max()) {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        std::cout<<"done"<<std::endl;
        return; // we are totally done
      }
    }

    seq = m_seq++;
  }
  //maintain the sequence number list
  if(!sequence_to_send.empty() && !retx_flag){
    seq = sequence_to_send.front();
    sequence_to_send.pop_front();
    }


  std::string interestname ;
  // std::string machinetosend = std::to_string(prefix_to_send);
  std::string machinetosend;
  interestname.append("/");
  // interestname.append(machinetosend);
  
  //shared_ptr<Name> nameWithSequence = make_shared<Name>(interestname);
  shared_ptr<Name> nameWithSequence;
  //
  if(retx_flag == true){
    u_int32_t retx_machine;
    if(seq >= 0 && seq <  uint32_t((total_machine-1)*model_size)){
      retx_machine = (seq%(total_machine-1)+m_rank+1)%total_machine;

      machinetosend = std::to_string(retx_machine);
      interestname.append(machinetosend);
      nameWithSequence = make_shared<Name>(interestname);

      nameWithSequence->appendSequenceNumber(seq);
      nameWithSequence->append("ScatterReduce");
      nameWithSequence->appendNumber(m_rank);

      nameWithSequence->appendNumber(seq%(total_machine-1));
      
    }
    else if(seq >= uint32_t((total_machine-1)*model_size) && seq <  uint32_t((total_machine-1)*(model_size+1))){
      retx_machine = (seq - (total_machine-1)*model_size + m_rank + 1)%total_machine;

      machinetosend = std::to_string(retx_machine);
      interestname.append(machinetosend);
      nameWithSequence = make_shared<Name>(interestname);

      nameWithSequence->appendSequenceNumber(seq);
      nameWithSequence->append("AllGatherSignal");
      nameWithSequence->appendNumber(m_rank);

      nameWithSequence->appendNumber(seq - (total_machine-1)*model_size);
    }
    else{
      retx_machine = (seq - (total_machine-1)*(model_size+1))/model_size;

      machinetosend = std::to_string(retx_machine);
      interestname.append(machinetosend);
      nameWithSequence = make_shared<Name>(interestname);

      nameWithSequence->appendSequenceNumber(seq);
      nameWithSequence->append("AllGatherPull");
      //nameWithSequence->appendNumber(m_rank);
      //when AllGather_Pull, dont need to append m_rank

      nameWithSequence->appendNumber((seq - (total_machine-1)*(model_size+1))%model_size);
    }
  }
  else{
    machinetosend = std::to_string(prefix_to_send);
    interestname.append(machinetosend);
    nameWithSequence = make_shared<Name>(interestname);
    if(flag == 0){
      nameWithSequence->appendSequenceNumber(seq);
      nameWithSequence->append("ScatterReduce");
      nameWithSequence->appendNumber(m_rank);
      }
    else if(flag == 1){
      nameWithSequence->appendSequenceNumber(seq = (total_machine-1)*model_size+data_sequence);
      nameWithSequence->append("AllGatherSignal");
      nameWithSequence->appendNumber(m_rank);
      }
    else if(flag == 2){
      nameWithSequence->appendSequenceNumber(seq = (total_machine-1)*(model_size+1)+(prefix_to_send*model_size)+data_sequence);
      nameWithSequence->append("AllGatherPull");
      //nameWithSequence->appendNumber(m_rank);
      //when AllGather_Pull, dont need to append m_rank
      }
    nameWithSequence->appendNumber(data_sequence);
  }
  
  // std::string kwd = std::to_string(m_rank);
  // nameWithSequence->appendKeyword("kwd");

  // nameWithSequence->appendNumber(data_sequence);
  
  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  // if(flag == 2)
  //   interest->getNonce()
  //std::cout<<interest->getName()<<std::endl;
  interest->setCanBePrefix(false);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);
  interest->setRank(m_rank);
  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << seq);

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  //std::cout<<"enter send "<<GetNode()->GetId()<<std::endl;
  m_appLink->onReceiveInterest(*interest);
  //m_allgathersize(this->GetNode()->GetId(),interest->Gets);

  //ScheduleNextPacket();
  //std::cout<<"enter send"<<GetNode()->GetId()<<std::endl;
}

void
ndnml::SendPacket2()
{
  if (!m_active){
    std::cout<<"no active"<<std::endl;
    return;}

  NS_LOG_FUNCTION_NOARGS();

  bool retx_flag = false;
  uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid
  //std::cout<<"retrans size: "<<m_retxSeqs.size()<<std::endl;
  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    std::cout<<"enter retrans"<<seq<<std::endl;
    m_retxSeqs.erase(m_retxSeqs.begin());
    retx_flag = true;
    break;
  }


  if (seq == std::numeric_limits<uint32_t>::max()) {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        std::cout<<"done"<<std::endl;
        return; // we are totally done
      }
    }

    seq = m_seq++;
  }
  //maintain the sequence number list
  if(sequence_to_send.empty()&&!retx_flag)
    return;

  if(!retx_flag){
  seq = sequence_to_send.front();
  sequence_to_send.pop_front();
  }

  std::string interestname ;
  // std::string machinetosend = std::to_string(prefix_to_send);
  std::string machinetosend;
  interestname.append("/");
  // interestname.append(machinetosend);
  
  //shared_ptr<Name> nameWithSequence = make_shared<Name>(interestname);
  shared_ptr<Name> nameWithSequence;
  //

  u_int32_t retx_machine;
  if(seq >= 0 && seq <  uint32_t((total_machine-1)*model_size)){
    retx_machine = (seq%(total_machine-1)+m_rank+1)%total_machine;

    machinetosend = std::to_string(retx_machine);
    interestname.append(machinetosend);
    nameWithSequence = make_shared<Name>(interestname);

    nameWithSequence->appendSequenceNumber(seq);
    nameWithSequence->append("ScatterReduce");
    nameWithSequence->appendNumber(m_rank);

    nameWithSequence->appendNumber(seq%(total_machine-1));
    
  }
  else if(seq >= uint32_t((total_machine-1)*model_size) && seq <  uint32_t((total_machine-1)*(model_size+1))){
    retx_machine = (seq - (total_machine-1)*model_size + m_rank + 1)%total_machine;

    machinetosend = std::to_string(retx_machine);
    interestname.append(machinetosend);
    nameWithSequence = make_shared<Name>(interestname);

    nameWithSequence->appendSequenceNumber(seq);
    nameWithSequence->append("AllGatherSignal");
    nameWithSequence->appendNumber(m_rank);

    nameWithSequence->appendNumber(seq - (total_machine-1)*model_size);
  }
  else{
    retx_machine = (seq - (total_machine-1)*(model_size+1))/model_size;

    machinetosend = std::to_string(retx_machine);
    interestname.append(machinetosend);
    nameWithSequence = make_shared<Name>(interestname);

    nameWithSequence->appendSequenceNumber(seq);
    nameWithSequence->append("AllGatherPull");
    //nameWithSequence->appendNumber(m_rank);
    //when AllGather_Pull, dont need to append m_rank

    nameWithSequence->appendNumber((seq - (total_machine-1)*(model_size+1))%model_size);
  }
    // machinetosend = std::to_string(prefix_to_send);
    // interestname.append(machinetosend);
    // nameWithSequence = make_shared<Name>(interestname);
    // if(flag == 0){
    //   nameWithSequence->appendSequenceNumber(seq);
    //   nameWithSequence->append("ScatterReduce");
    //   nameWithSequence->appendNumber(m_rank);
    //   }
    
    // else if(flag == 1){
    //   nameWithSequence->appendSequenceNumber(seq = (total_machine-1)*model_size+data_sequence);
    //   nameWithSequence->append("AllGatherSignal");
    //   nameWithSequence->appendNumber(m_rank);
    //   }
    // else if(flag == 2){
    //   nameWithSequence->appendSequenceNumber(seq = (total_machine-1)*(model_size+1)+(prefix_to_send*model_size)+data_sequence);
    //   nameWithSequence->append("AllGatherPull");
    //   //nameWithSequence->appendNumber(m_rank);
    //   //when AllGather_Pull, dont need to append m_rank
    //   }
    // nameWithSequence->appendNumber(data_sequence);

  
  // std::string kwd = std::to_string(m_rank);
  // nameWithSequence->appendKeyword("kwd");

  // nameWithSequence->appendNumber(data_sequence);
  
  // shared_ptr<Interest> interest = make_shared<Interest> ();
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  // if(flag == 2)
  //   interest->getNonce()
  //std::cout<<interest->getName()<<std::endl;
  interest->setCanBePrefix(false);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);
  interest->setRank(m_rank);
  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << seq);

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  //std::cout<<"enter send "<<GetNode()->GetId()<<std::endl;
  m_appLink->onReceiveInterest(*interest);
  //m_allgathersize(this->GetNode()->GetId(),interest->Gets);
  SchedulePacketWithWindow();
  //ScheduleNextPacket();
  //std::cout<<"enter send"<<GetNode()->GetId()<<std::endl;
  
}

void
ndnml::SchedulePacket2(){
  SendPacket2();
}

void
ndnml::SchedulePacketWithWindow(){
    if (m_window == static_cast<uint32_t>(0)) {
    Simulator::Remove(m_sendEvent);

    NS_LOG_DEBUG(
      "Next event in " << (std::min<double>(0.5, m_rtt->RetransmitTimeout().ToDouble(Time::S)))
                       << " sec");
    m_sendEvent =
      Simulator::Schedule(Seconds(
                            std::min<double>(0.5, m_rtt->RetransmitTimeout().ToDouble(Time::S))),
                          &ndnml::SendPacket2, this);
  }
  else if (m_inFlight >= m_window) {
    // simply do nothing
  }
  else {
    if (m_sendEvent.IsRunning()) {
      Simulator::Remove(m_sendEvent);
    }

    m_sendEvent = Simulator::ScheduleNow(&ndnml::SendPacket2,this);
  }
}


///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ndnml::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;
  //std::cout<<"enter ondata "<<GetNode()->GetId()<<"\t";
  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  // This could be a problem......
  //std::cout<<data->getName()<<'\t';
  uint32_t seq = data->getName().at(1).toSequenceNumber();
  NS_LOG_INFO("< DATA for " << seq);

  uint64_t machine = std::stoi(data->getName().at(0).toUri());
  //std::cout<< machine<<std::endl;
  std::string phase = data->getName().at(2).toUri();
  //std::cout<< test1<<std::endl;
  //uint32_t test = data->getName().at(3).toNumber();
  //std::cout<< test<<std::endl;
  int hopCount = 0;
  auto hopCountTag = data->getTag<lp::HopCountTag>();
  if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
    hopCount = *hopCountTag;
  }
  NS_LOG_DEBUG("Hop count: " << hopCount);
  //std::cout<<"hop count: "<<hopCount<<std::endl;

  
  if(phase == "ScatterReduce"){
    int flag1 = 0;
    //std::cout<<"scatter check"<<std::endl;
    data_count[machine]++;
    for(int i = 0;i<total_machine;i++){
      if(data_count[i] != model_size && i != m_rank){
        flag1 = 1;
        break;
      }
    }
    if(flag1 == 0)
      Simulator::ScheduleNow(&ndnml::Allgather_Send,this);
  }
  else if(phase == "AllGatherPull"){
    int flag2 = 0;
    all_data_count[machine]++;
    //std::cout<<"AllGather_Pull count in machine "<<GetNode()->GetId()<<std::endl;
    for(int i = 0;i<total_machine;i++){
      if(all_data_count[i] != model_size && i != m_rank){
        flag2 = 1;
        break;
      }
    }
    if(flag2 == 0){
      Time now = Simulator::Now();
      std::cout<<"counts down "<<GetNode()->GetId()<<"communication time :"<<now - start_time<<std::endl;
      }
  }
  

  SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find(seq);
  if (entry != m_seqLastDelay.end()) {
    m_lastRetransmittedInterestDataDelay(this, seq, Simulator::Now() - entry->time, hopCount);
  }

  entry = m_seqFullDelay.find(seq);
  if (entry != m_seqFullDelay.end()) {
    m_firstInterestDataDelay(this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount);
  }

  m_seqRetxCounts.erase(seq);
  m_seqFullDelay.erase(seq);
  m_seqLastDelay.erase(seq);

  m_seqTimeouts.erase(seq);
  m_retxSeqs.erase(seq);

  m_rtt->AckSeq(SequenceNumber32(seq));

  //ndn-consumer-pcon's OnData
  if(m_highData < seq){
    if(seq < m_seqgapbase)
      m_highData = seq;
    else
      m_highData = uint32_t(seq - model_size);
  }

  if (data->getCongestionMark() > 0) {
    if (m_reactToCongestionMarks) {
      NS_LOG_DEBUG("Received congestion mark: " << data->getCongestionMark());
      //std::cout << "congestion mark triggered: " << data->getCongestionMark()<<"machine : "<<this->GetNode()->GetId()<<std::endl;
      WindowDecrease();
    }
    else {
      NS_LOG_DEBUG("Ignored received congestion mark: " << data->getCongestionMark());
    }
  }
  else {
    WindowIncrease();
  }

  if (m_inFlight > static_cast<uint32_t>(0)) {
    m_inFlight--;
  }

  NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);

  SchedulePacketWithWindow();
}

void
ndnml::WindowDecrease(){
  if (!m_useCwa || m_highData > m_recPoint) {
    const double diff = m_seq - m_highData;
    BOOST_ASSERT(diff > 0);

    m_recPoint = m_seq + (m_addRttSuppress * diff);

    CubicDecrease();

    if (m_window < m_initialWindow) {
      m_window = m_initialWindow;
    }
  }

  else {
    NS_LOG_DEBUG("Window decrease suppressed, HighData: " << m_highData << ", RecPoint: " << m_recPoint);
  }
}

void
ndnml::WindowIncrease()
{
    CubicIncrease();
}

void
ndnml::CubicIncrease()
{
  // 1. Time since last congestion event in Seconds
  const double t = time::duration_cast<time::microseconds>(
                     time::steady_clock::now() - m_cubicLastDecrease).count() / 1e6;

  // 2. Time it takes to increase the window to cubic_wmax
  // K = cubic_root(W_max*(1-beta_cubic)/C) (Eq. 2)
  const double k = std::cbrt(m_cubicWmax * (1 - m_cubicBeta) / CUBIC_C);

  // 3. Target: W_cubic(t) = C*(t-K)^3 + W_max (Eq. 1)
  const double w_cubic = CUBIC_C * std::pow(t - k, 3) + m_cubicWmax;

  // 4. Estimate of Reno Increase (Currently Disabled)
  //  const double rtt = m_rtt->GetCurrentEstimate().GetSeconds();
  //  const double w_est = m_cubic_wmax*m_beta + (3*(1-m_beta)/(1+m_beta)) * (t/rtt);
  constexpr double w_est = 0.0;

  // Actual adaptation
  if (m_window < m_ssthresh) {
    m_window += 1.0;
  }
  else {
    BOOST_ASSERT(m_cubicWmax > 0);

    double cubic_increment = std::max(w_cubic, w_est) - m_window;
    // Cubic increment must be positive:
    // Note: This change is not part of the RFC, but I added it to improve performance.
    if (cubic_increment < 0) {
      cubic_increment = 0.0;
    }
    m_window += cubic_increment / m_window;
  }
}

void
ndnml::CubicDecrease()
{
  // This implementation is ported from https://datatracker.ietf.org/doc/rfc8312/

  const double FAST_CONV_DIFF = 1.0; // In percent
  //std::cout << "CubicDecrease" << std::endl;

  // A flow remembers the last value of W_max,
  // before it updates W_max for the current congestion event.

  // Current w_max < last_wmax
  if (m_useCubicFastConv && m_window < m_cubicLastWmax * (1 - FAST_CONV_DIFF / 100)) {
    //std::cout << "Fast Convergence" << std::endl;
    m_cubicLastWmax = m_window;
    m_cubicWmax = m_window * (1.0 + m_cubicBeta) / 2.0;
  }
  else {
    // Save old cwnd as w_max:
    m_cubicLastWmax = m_window;
    m_cubicWmax = m_window;
  }

  m_ssthresh = m_window * m_cubicBeta;
  m_ssthresh = std::max<double>(m_ssthresh, m_initialWindow);
  m_window = m_ssthresh;

  m_cubicLastDecrease = time::steady_clock::now();
}


void
ndnml::OnNack(shared_ptr<const lp::Nack> nack)
{
  /// tracing inside
  App::OnNack(nack);

  NS_LOG_INFO("NACK received for: " << nack->getInterest().getName()
              << ", reason: " << nack->getReason());
}

void
ndnml::OnTimeout(uint32_t sequenceNumber)
{ //ndn-consumer-window
  NS_LOG_FUNCTION(sequenceNumber);
   WindowDecrease();

  if (m_inFlight > static_cast<uint32_t>(0)) {
    m_inFlight--;
  }

  NS_LOG_DEBUG("Window: " << m_window << ", InFlight: " << m_inFlight);

  // std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
  // m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  m_rtt->IncreaseMultiplier(); // Double the next RTO
  m_rtt->SentSeq(SequenceNumber32(sequenceNumber),
                 1); // make sure to disable RTT calculation for this sample
  m_retxSeqs.insert(sequenceNumber);
  std::cout<<Simulator::Now()<<std::endl;
  std::cout<<"enter time out"<<GetNode()->GetId()<<"for "<<sequenceNumber<<std::endl;
  // if(sequenceNumber<(total_machine-1)*model_size)
  //   ScheduleNextPacket(m_rank+(sequenceNumber%(total_machine-1)));
  // else
  //   ScheduleNextPacket(sequenceNumber-(total_machine-1)*model_size);
   SchedulePacketWithWindow();
}

void
ndnml::WillSendOutInterest(uint32_t sequenceNumber)
{
  //ndn-consumer-window
  m_inFlight++;

  NS_LOG_DEBUG("Trying to add " << sequenceNumber << " with " << Simulator::Now() << ". already "
                                << m_seqTimeouts.size() << " items");

  m_seqTimeouts.insert(SeqTimeout(sequenceNumber, Simulator::Now()));
  m_seqFullDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqLastDelay.erase(sequenceNumber);
  m_seqLastDelay.insert(SeqTimeout(sequenceNumber, Simulator::Now()));

  m_seqRetxCounts[sequenceNumber]++;

  m_rtt->SentSeq(SequenceNumber32(sequenceNumber), 1);
}


void
ndnml::train_process(){
    sleep(1);
    ScatterReduce(1);
    Allgather();
}

void
ndnml::Allgather(){};

void
ndnml::Allgather_Send(){
    //std::cout<<"enter allgather_send for "<<GetNode()->GetId()<<std::endl;
        // for(int i = 1;i<total_machine;i++){
            SchedulePacket(1,(m_rank+1)%total_machine,1-1);
  // }
}
void
ndnml::Allgather_Pull(uint32_t machine){
    //std::cout<<"enter allgather_pull in machine "<<GetNode()->GetId()<<"for machine "<<machine<<std::endl;
    // for(int j = 0;j<model_size;j++){
            SchedulePacket(2,machine,1);
    // }
}

void
ndnml::SchedulePacket(const uint32_t flag,const uint32_t machinenumber,const uint32_t sequenceNumber)
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";
  // if (m_firstTime) {
  //   m_sendEvent = Simulator::Schedule(Seconds(0.0), &Consumer::SendPacket, this);
  //   m_firstTime = false;
  //   m_sent_packet++;

  // }
  // else if (!m_sendEvent.IsRunning() && m_sent_packet < m_frequency){
  //   // m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
  //   //                                                   : Seconds(m_random->GetValue()),
  //   //                                   &Consumer::SendPacket, this);
  //   m_sendEvent = Simulator::Schedule(Seconds(0.0) , &Consumer::SendPacket, this);
  //   m_sent_packet++;

  // }
  // else if(!m_sendEvent.IsRunning() && m_sent_packet == m_frequency){
  //   m_sendEvent = Simulator::Schedule(Seconds(1.0) , &Consumer::SendPacket, this);
  //   m_sent_packet = 0;
  // }
//   if (scatter_flag) {
//     m_sendEvent = Simulator::Schedule(Seconds(0.0), &Consumer::SendPacket, this);
//     m_firstTime = false;
//   }
//   else if (allgather_flag && !m_sendEvent.IsRunning())
//     m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
//                                                       : Seconds(m_random->GetValue()),
//                                       &Consumer::SendPacket, this);
    //if(scatter_flag)
  // std::cout<<"enter time out schedule"<<GetNode()->GetId()<<std::endl;
  // uint32_t machinenumber = (m_rank+(sequenceNumber%(total_machine-1)))%total_machine;
  if(flag == 0 ){
    last_send_time = 0.0;
    for(int j = 0;j<model_size;j++){
        for(int i = 1;i<total_machine;i++){
            //SendPacket1(0,(i+m_rank)%total_machine,j);
            // m_sendEvent = Simulator::Schedule(Seconds(last_send_time), &ndnml::SendPacket1, this, flag, (i+m_rank)%total_machine, j);
            sequence_to_send.push_back(i-1+j*(total_machine-1));
            
            //last_send_time += 1.0 / m_frequency;
        }
    }
    SchedulePacketWithWindow();
    // if (m_firstTime) {
    //   m_sendEvent = Simulator::Schedule(Seconds(0.0), &ndnml::SendPacket1, this, flag, machinenumber, sequenceNumber);
    //   m_firstTime = false;
    //   last_send_time = 0.0;
    // }
    // else if (!m_sendEvent.IsRunning()){
    //   last_send_time += 1.0 / m_frequency;
    //   m_sendEvent = Simulator::Schedule(Seconds(last_send_time),                                                 
    //                                     &ndnml::SendPacket1, this, flag, machinenumber, sequenceNumber);
    // }
  }

  if(flag == 1 ){
    last_send_time = 0.0;
    for(int i = 1;i<total_machine;i++){
            //SendPacket1(1,(m_rank+i)%total_machine,i-1);
            //m_sendEvent = Simulator::Schedule(Seconds(last_send_time), &ndnml::SendPacket1, this, flag, (m_rank+i)%total_machine, i-1);
            sequence_to_send.push_back(i-1+model_size*(total_machine-1));
            //last_send_time += 1.0 / m_frequency;
    }
    SchedulePacketWithWindow();
  }

   if(flag == 2 ){
    last_send_time = 0.0;
    for(int j = 0;j<model_size;j++){
            //SendPacket1(2,machinenumber,j);
            //m_sendEvent = Simulator::Schedule(Seconds(last_send_time), &ndnml::SendPacket1, this, flag, machinenumber, j);
            sequence_to_send.push_back(j+(model_size+1)*(total_machine-1)+machinenumber*model_size);
            //last_send_time += 1.0 / m_frequency;
    }
    SchedulePacketWithWindow();
  }

}
void
ndnml::ScheduleNextPacket(uint32_t sequenceNumber)
{
  // double mean = 8.0 * m_payloadSize / m_desiredRate.GetBitRate ();
  // std::cout << "next: " << Simulator::Now().ToDouble(Time::S) + mean << "s\n";
  // if (m_firstTime) {
  //   m_sendEvent = Simulator::Schedule(Seconds(0.0), &Consumer::SendPacket, this);
  //   m_firstTime = false;
  //   m_sent_packet++;

  // }
  // else if (!m_sendEvent.IsRunning() && m_sent_packet < m_frequency){
  //   // m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
  //   //                                                   : Seconds(m_random->GetValue()),
  //   //                                   &Consumer::SendPacket, this);
  //   m_sendEvent = Simulator::Schedule(Seconds(0.0) , &Consumer::SendPacket, this);
  //   m_sent_packet++;

  // }
  // else if(!m_sendEvent.IsRunning() && m_sent_packet == m_frequency){
  //   m_sendEvent = Simulator::Schedule(Seconds(1.0) , &Consumer::SendPacket, this);
  //   m_sent_packet = 0;
  // }
//   if (scatter_flag) {
//     m_sendEvent = Simulator::Schedule(Seconds(0.0), &Consumer::SendPacket, this);
//     m_firstTime = false;
//   }
//   else if (allgather_flag && !m_sendEvent.IsRunning())
//     m_sendEvent = Simulator::Schedule((m_random == 0) ? Seconds(1.0 / m_frequency)
//                                                       : Seconds(m_random->GetValue()),
//                                       &Consumer::SendPacket, this);
    //if(scatter_flag)
  std::cout<<"enter time out schedule"<<GetNode()->GetId()<<std::endl;
  uint32_t machinenumber = (m_rank+(sequenceNumber%(total_machine-1)))%total_machine;
  SendPacket1(0,machinenumber,sequenceNumber);
    //ndnml::ScatterReduce(1);
}

void
ndnml::ScatterReduce(uint32_t iii){
    //std::cout << iii << std::endl;
    if (!m_active)
        return;
    data_count.clear();
    data_count.assign(total_machine,0);
    all_data_count.clear();
    all_data_count.assign(total_machine,0);
    //std::cout<<"enter reduce"<<GetNode()->GetId()<<std::endl;
    // for(int j = 0;j<model_size;j++){
    //     for(int i = 1;i<total_machine;i++){
            SchedulePacket(0,(1+m_rank)%total_machine,1);
    //     }
    // }
    return;
    //SendPacket(0,GetNode()->GetId()+1,100);
}

void
ndnml::train(){
    std::cout<<"enter train"<<GetNode()->GetId()<<std::endl;
    start_time = Simulator::Now();
    //event_id = Simulator::Schedule(Seconds(1.0), &ndnml::ScatterReduce,this, 1);
    ScatterReduce(1);
}




void
ndnml::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active){
    std::cout<<"machine: "<<GetNode()->GetId()<<"not active"<<std::endl;
    return;}

  //std::cout<<"machine "<<GetNode()->GetId()<<" get interest "<<interest->getName()<<" Rank: "<<interest->getRank()<<std::endl;

  uint32_t machine = interest->getName().at(3).toNumber();
  std::string phaze = interest->getName().at(2).toUri();
 
  //std::cout<<interest->getName().at(2).toNumber()<<std::endl;
  Name dataName(interest->getName());
  // dataName.append(m_postfix);
  // dataName.appendVersion();
  //dataName.set(3,Name::Component::fromNumber(m_rank));
  //dataName.appendNumber(m_rank);
  //std::cout<<dataName<<std::endl;
  auto data = make_shared<ndn::Data>(dataName);
  //data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));

  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  data->setSignatureInfo(signatureInfo);

  ::ndn::EncodingEstimator estimator;
  ::ndn::EncodingBuffer encoder(estimator.appendVarNumber(m_signature), 0);
  encoder.appendVarNumber(m_signature);
  data->setSignatureValue(encoder.getBuffer());

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

  // to create real wire encoding
  data->wireEncode();
  m_transmittedDatas(data, this, m_face);
  m_appLink->onReceiveData(*data);
  m_allgathersize(this->GetNode()->GetId(),data->getContent().size());
  if(phaze =="AllGatherPull"){
     //m_allgathersize(this->GetNode()->GetId(),data->getContent().size());
     //std::cout<<"rank "<<interest->getApplicationParameters().get(0).data();
  }
  if(phaze =="AllGatherSignal"){
    Allgather_Pull(machine);
  }
}

} // namespace ndn
} // namespace ns3
