/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  The University of Memphis
 *
 * This file is part of NDNSD.
 * Author: Saurab Dulal (sdulal@memphis.edu)
 *
 * NDNSD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU Lesser General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NDNSD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along with
 * NDNSD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#ifndef NDNSD_SERVICE_DISCOVERY_HPP
#define NDNSD_SERVICE_DISCOVERY_HPP

#include "file-processor.hpp"

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/util/scheduler.hpp>
#include <ndn-cxx/util/time.hpp>
#include <ndn-cxx/util/dummy-client-face.hpp>

#include <ndn-cxx/encoding/block-helpers.hpp>

#include <ndn-svs/svspubsub.hpp>

#include <iostream>

#include <thread>

using namespace ndn::time_literals;

namespace ndnsd {
namespace discovery {
namespace tlv {

  enum {
    DiscoveryData = 128,
    ServiceInfo = 129,
    ServiceStatus = 130,
    Name = 131,               // New TLV type for serviceName
    ApplicationPrefix = 132,  // New TLV type for applicationPrefix
    ServiceLifetime = 133,    // New TLV type for serviceLifetime
    PublishTimestamp = 134,   // New TLV type for publishTimestamp
    ServiceMetaInfo = 135,    // New TLV type for serviceMetaInfo
    Key = 136,                // New TLV type for keys in serviceMetaInfo
    Value = 137,               // New TLV type for values in serviceMetaInfo
    KeyValuePair = 138         // New TLV type for key-value pairs in serviceMetaInfo
  };

} // namespace tlv

enum {
  OPTIONAL = 0,
  REQUIRED = 1,
};

// service status
enum {
  EXPIRED = 0,
  ACTIVE = 1,
};

/**
  each node will list on NDNSD_RELOAD_PREFIX, and will update their service once the
  interest is received.
**/
// const char* NDNSD_RELOAD_PREFIX; = "/ndnsd/reload";
extern uint32_t RETRANSMISSION_COUNT;


class Error : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;
};
struct Details
{
  ndn::Name serviceName;
  ndn::Name applicationPrefix;
  int serviceLifetime;
  time_t publishTimestamp;
  std::map<std::string, std::string> serviceMetaInfo;

  // Function to decode an NDN Block into a Details object
  static Details decode(const ndn::Block& block)
  {
    Details details;
    if (block.type() != tlv::ServiceInfo) {
      throw Error("Invalid TLV type");
    }
    block.parse();

    for (const auto& element : block.elements()) {
      switch (element.type()) {
        case tlv::Name:
          details.serviceName = ndn::Name(ndn::readString(element));
          break;
        case tlv::ApplicationPrefix:
          details.applicationPrefix = ndn::Name(ndn::readString(element));
          break;
        case tlv::ServiceLifetime:
          details.serviceLifetime = ndn::readNonNegativeInteger(element);
          break;
        case tlv::PublishTimestamp:
          details.publishTimestamp = ndn::readNonNegativeInteger(element);
          break;
        case tlv::ServiceMetaInfo:
          element.parse();
          for (const auto& keyValueElement : element.elements()) {
            keyValueElement.parse();
            std::string key = ndn::readString(keyValueElement.get(tlv::Key));
            std::string value = ndn::readString(keyValueElement.get(tlv::Value));
            details.serviceMetaInfo[key] = value;
          }
          break;
        default:
          throw Error("Unknown TLV type");
      }
    }

    return details;
  }

  // Function to encode a Details object into an NDN Block
  ndn::Block encode() const
  {
    ndn::Block buffer(tlv::ServiceInfo);

    if (!serviceName.empty()) {
      buffer.push_back(ndn::makeStringBlock(tlv::Name, serviceName.toUri()));
    }
    if (!applicationPrefix.empty()) {
      buffer.push_back(ndn::makeStringBlock(tlv::ApplicationPrefix, applicationPrefix.toUri()));
    }
    buffer.push_back(ndn::makeNonNegativeIntegerBlock(tlv::ServiceLifetime, serviceLifetime));
    buffer.push_back(ndn::makeNonNegativeIntegerBlock(tlv::PublishTimestamp, publishTimestamp));
    ndn::Block metaInfoBuffer(tlv::ServiceMetaInfo);
    for (const auto& [key, value] : serviceMetaInfo) {
      ndn::Block keyValuePair(tlv::KeyValuePair);
      keyValuePair.push_back(ndn::makeStringBlock(tlv::Key, key));
      keyValuePair.push_back(ndn::makeStringBlock(tlv::Value, value));
      metaInfoBuffer.push_back(keyValuePair);
    }
    buffer.push_back(metaInfoBuffer);
    buffer.encode();
    return buffer;
  }

  std::string toString() const
  {
    std::stringstream ss;
    ss << "ServiceName: " << serviceName << "\n";
    ss << "ApplicationPrefix: " << applicationPrefix << "\n";
    ss << "ServiceLifetime: " << serviceLifetime << "\n";
    ss << "PublishTimestamp: " << publishTimestamp << "\n";
    ss << "ServiceMetaInfo: \n";
    for (const auto& [key, value] : serviceMetaInfo) {
      ss << key << ": " << value << "\n";
    }
    return ss.str();
  }
};



typedef std::function<void(const Details& serviceUpdates)> DiscoveryCallback;


class ServiceDiscovery
{

public:

  /**
    @brief constructor

    required
    {
      serviceName printer
      appPrefix /printer1/service-info
      lifetime 100
    }
    details
    {
      description "Hp Ledger Jet"
      make "2016"
    }
    ; all the keys in required field needs to have value
    ; the details can have as many key-values are needed

    @param servicegroupName The sync group that publishes the service info
    @param discoveryCallback
  **/
  ServiceDiscovery(const ndn::Name& servicegroupName,
                    const ndn::Name& nodeName,
                    ndn::Face& face,
                    ndn::KeyChain& keyChain,
                    const DiscoveryCallback& discoveryCallback);
  

  // destructor
  ~ServiceDiscovery();

  void
  publishServiceDetail(Details details);

  std::map<std::string, Details>
  getReceivedServiceDetails(){
    return m_receivedDetails;
  }

private:
  void
  run();

  /*
    @brief Cancel all pending operations, close connection to forwarder
    and stop the ioService.
  */
  void
  stop();

  void
  OnServiceUpdate(const ndn::svs::SVSPubSub::SubscriptionData &subscription);

  void
  OnServiceDiscovery(const ndn::svs::SVSPubSub::SubscriptionData &subscription);

public:
  uint8_t m_appType;
  Details m_producerState;

private:
  ndn::Face& m_face;
  ndn::KeyChain& m_keyChain;
  std::shared_ptr<ndn::svs::SVSPubSub> m_svsps;

  const std::string m_filename;
  ServiceInfoFileProcessor m_fileProcessor;
  ndn::Name m_servicegroupName;
  ndn::Name m_nodeName;

  // cache the details in a map
  std::map<std::string, Details> m_serviceDetails;

  // cache recevied details in a map
  std::map<std::string, Details> m_receivedDetails;

  DiscoveryCallback m_discoveryCallback;

  ndn::time::steady_clock::time_point m_lastDiscoveryTime;
};

} //namespace discovery
} //namespace ndnsd
#endif // NDNSD_SERVICE_DISCOVERY_HPP
