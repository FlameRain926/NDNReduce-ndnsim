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

#include "ndn-consumer-ml.hpp"
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
#include <ndn-cxx/lp/tags.hpp>

NS_LOG_COMPONENT_DEFINE("ndn.ConsumerML");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ConsumerML);

TypeId
ConsumerML::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ConsumerML")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<ConsumerML>()

      .AddAttribute("MachineRank", "Frequency of interest packets", StringValue("0"),
                    MakeIntegerAccessor(&ConsumerML::m_rank), MakeIntegerChecker<int>())
      .AddAttribute("TotalMachineNumber", "Frequency of interest packets", StringValue("1"),
                    MakeIntegerAccessor(&ConsumerML::totle_machine), MakeIntegerChecker<int>())
      .AddAttribute("ModelSize", "Frequency of interest packets", StringValue("20"),
                    MakeIntegerAccessor(&ConsumerML::model_size), MakeIntegerChecker<int>())

    ;

  return tid;
}

ConsumerML::ConsumerML()
  : totle_machine(4),
    scatter_flag(true),
    allgather_flag(false)
{
  NS_LOG_FUNCTION_NOARGS();
  m_seqMax = std::numeric_limits<uint32_t>::max();
}

ConsumerML::~ConsumerML()
{
}

void
ConsumerML::ScheduleNextPacket()
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
    std::cout<<"enter here"<<std::endl;
    ConsumerML::ScatterReduce();
}

void
ConsumerML::ScatterReduce(){
    if (!m_active)
    return;
    data_count.clear();
    data_count.assign(totle_machine,0);
    std::cout<<"enter reduce"<<std::endl;
    SendPacket(0,1,101);
}

void
ConsumerML::Allgather(){
    
}

void
ConsumerML::SendPacket(const bool flag,const int machine_to_send,const int data_sequence)
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

  std::string interestname ;
  std::string machinetosend = std::to_string(machine_to_send);
  interestname.append("/");
  interestname.append(machinetosend);
  //
  shared_ptr<Name> nameWithSequence = make_shared<Name>(interestname);
  nameWithSequence->appendSequenceNumber(seq);
  nameWithSequence->appendNumber(data_sequence);
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

  //ScheduleNextPacket();
  std::cout<<"enter send"<<std::endl;
}

void
ConsumerML::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;
  std::cout<<"enter ml"<<std::endl;
  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

  // NS_LOG_INFO ("Received content object: " << boost::cref(*data));

  // This could be a problem......
  uint32_t seq = data->getName().at(1).toSequenceNumber();
  NS_LOG_INFO("< DATA for " << seq);

  uint32_t test = data->getName().at(2).toNumber();
  std::cout<< test<<std::endl;
  int hopCount = 0;
  auto hopCountTag = data->getTag<lp::HopCountTag>();
  if (hopCountTag != nullptr) { // e.g., packet came from local node's cache
    hopCount = *hopCountTag;
  }
  NS_LOG_DEBUG("Hop count: " << hopCount);

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
}



} // namespace ndn
} // namespace ns3
