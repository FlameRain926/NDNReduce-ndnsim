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

#ifndef NDN_CONSUMER_H
#define NDN_CONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"

#include <set>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

/*consumer-window*/
#include "ns3/traced-value.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief NDN application for sending out Interest packets
 */
class ndnml : public App {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  ndnml();
  virtual ~ndnml(){};

  // From App
  virtual void
  OnData(shared_ptr<const Data> contentObject);

  // From App
  virtual void
  OnNack(shared_ptr<const lp::Nack> nack);

  /**
   * @brief Timeout event
   * @param sequenceNumber time outed sequence number
   */
  virtual void
  OnTimeout(uint32_t sequenceNumber);

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  /**
   * @brief Actually send packet
   */
  void
  SendPacket();

  void
  SendPacket1(const int flag,const int machine_to_send,const int data_sequence);

  void
  SendPacket2();

  /**
   * @brief An event that is fired just before an Interest packet is actually send out (send is
   *inevitable)
   *
   * The reason for "before" even is that in certain cases (when it is possible to satisfy from the
   *local cache),
   * the send call will immediately return data, and if "after" even was used, this after would be
   *called after
   * all processing of incoming data, potentially producing unexpected results.
   */
  virtual void
  WillSendOutInterest(uint32_t sequenceNumber);

public:
  typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
  typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);
  static u_int16_t all_scatter_fin;
protected:
  // from App
  virtual void
  StartApplication();

  virtual void
  StopApplication();

  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket(uint32_t sequenceNumber);

  void
  SchedulePacket(const u_int32_t flag,const uint32_t machinenumber,const uint32_t sequenceNumber);

  void
  SchedulePacket2();

  //ndn-consumer-window
  void
  SchedulePacketWithWindow();

  void
  WindowIncrease();

  void
  WindowDecrease(bool loss);

  void
  CubicIncrease();

  void
  CubicDecrease();

  void
  WindowReset();

  /**
   * \brief Checks if the packet need to be retransmitted becuase of retransmission timer expiration
   */
  void
  CheckRetxTimeout();

  /**
   * \brief Modifies the frequency of checking the retransmission timeouts
   * \param retxTimer Timeout defining how frequent retransmission timeouts should be checked
   */
  void
  SetRetxTimer(Time retxTimer);

  /**
   * \brief Returns the frequency of checking the retransmission timeouts
   * \return Timeout defining how frequent retransmission timeouts should be checked
   */
  Time
  GetRetxTimer() const;

  void
  train_process();

  void
  ScatterReduce(uint32_t iii);

  void
  Allgather();

  void
  Allgather_Send();

  void
  Allgather_Pull(uint32_t machine);

  void
  train();

protected:
  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator

  uint32_t m_seq;      ///< @brief currently requested sequence number
  uint32_t m_seqMax;   ///< @brief maximum number of sequence number
  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
  EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed

  Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator

  Time m_offTime;          ///< \brief Time interval between packets
  Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet

  //lxd consumer
  int model_size; // model size
  int m_rank; // machine rank
  int total_machine;//totle machine number
  bool scatter_flag;
  bool allgather_flag;
  int round;//training round
  std::vector<int> data_count;//received data count per round
  std::vector<int> all_data_count;//received data count per round
  std::vector<int> data_timeout_count;//timeouted data count per round
  EventId event_id;
  bool enter_allgather = false;
  u_int16_t scatfin_machine = 0;
  // static u_int16_t all_scatter_fin;

  std::list<int> sequence_to_send;//a list to maintain the pending sequence number

  double m_frequency;
  double last_send_time = 0;
  bool m_firstTime;

  //ndn-consumer-window
  uint32_t m_payloadSize; // expected payload size
  double m_maxSize;       // max size to request

  uint32_t m_initialWindow;
  bool m_setInitialWindowOnTimeout;

  TracedValue<double> m_window;
  TracedValue<uint32_t> m_inFlight;

  //ndn-consumer-pcon
  bool m_reactToCongestionMarks;
  double m_addRttSuppress;
  bool m_useCwa;

  double m_ssthresh;
  uint32_t m_highData;
  double m_recPoint;
  uint32_t m_seqgapbase;//base for allgather pull

  // TCP CUBIC Parameters //
  static constexpr double CUBIC_C = 0.4;
  bool m_useCubicFastConv;
  double m_cubicBeta;

  double m_cubicWmax;
  double m_cubicLastWmax;
  time::steady_clock::TimePoint m_cubicLastDecrease;
  Time m_cubicLastdec;

  //lxd producer
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  Time start_time;

  uint32_t m_signature;
  Name m_keyLocator;


  /// @cond include_hidden
  /**
   * \struct This struct contains sequence numbers of packets to be retransmitted
   */
  struct RetxSeqsContainer : public std::set<uint32_t> {
  };

  RetxSeqsContainer m_retxSeqs; ///< \brief ordered set of sequence numbers to be retransmitted

  /**
   * \struct This struct contains a pair of packet sequence number and its timeout
   */
  struct SeqTimeout {
    SeqTimeout(uint32_t _seq, Time _time)
      : seq(_seq)
      , time(_time)
    {
    }

    uint32_t seq;
    Time time;
  };
  /// @endcond

  /// @cond include_hidden
  class i_seq {
  };
  class i_timestamp {
  };
  /// @endcond

  /// @cond include_hidden
  /**
   * \struct This struct contains a multi-index for the set of SeqTimeout structs
   */
  struct SeqTimeoutsContainer
    : public boost::multi_index::
        multi_index_container<SeqTimeout,
                              boost::multi_index::
                                indexed_by<boost::multi_index::
                                             ordered_unique<boost::multi_index::tag<i_seq>,
                                                            boost::multi_index::
                                                              member<SeqTimeout, uint32_t,
                                                                     &SeqTimeout::seq>>,
                                           boost::multi_index::
                                             ordered_non_unique<boost::multi_index::
                                                                  tag<i_timestamp>,
                                                                boost::multi_index::
                                                                  member<SeqTimeout, Time,
                                                                         &SeqTimeout::time>>>> {
  };

  SeqTimeoutsContainer m_seqTimeouts; ///< \brief multi-index for the set of SeqTimeout structs

  SeqTimeoutsContainer m_seqLastDelay;
  SeqTimeoutsContainer m_seqFullDelay;
  std::map<uint32_t, uint32_t> m_seqRetxCounts;

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
                 uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

  TracedCallback<uint32_t, uint32_t> m_allgathersize;
  TracedCallback<uint32_t> m_scatterfinsign;
  TracedCallback<uint32_t> m_allgatherfinsign;
  TracedCallback<uint32_t, uint32_t,double,double,Time> m_timeout;
  TracedCallback<uint32_t, uint32_t,double,double,uint32_t> m_retrans;
  TracedCallback<bool, uint32_t,double,double,uint32_t> m_sendsign;
  TracedCallback<uint32_t,uint32_t,double,double,uint64_t,Time> m_hop;
  TracedCallback<double,double> m_increase;
  TracedCallback<double,double,double,double> m_decrease;
  /// @endcond
};

} // namespace ndn
} // namespace ns3

#endif
