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

#include "service-discovery.hpp"
#include <string>
#include <iostream>
#include <ndn-cxx/util/logger.hpp>

using namespace ndn::time_literals;

NDN_LOG_INIT(ndnsd.ServiceDiscovery);

namespace ndnsd {
namespace discovery {

using namespace ndn::svs;

ServiceDiscovery::ServiceDiscovery(const ndn::Name& servicegroupName, const ndn::Name& nodeName, 
                    ndn::Face& face,
                    ndn::KeyChain& keyChain,
                    const DiscoveryCallback& discoveryCallback) 
  : m_servicegroupName(servicegroupName)
  , m_face(face)
  , m_keyChain(keyChain)
  , m_nodeName(nodeName)
  , m_discoveryCallback(discoveryCallback)
{
    // Use HMAC signing for Sync Interests
    // Note: this is not generally recommended, but is used here for simplicity
    ndn::svs::SecurityOptions secOpts(m_keyChain);
    secOpts.interestSigner->signingInfo.setSigningHmacKey("dGhpcyBpcyBhIHNlY3JldCBtZXNzYWdl");

    // Sign data packets using SHA256 (for simplicity)
    secOpts.dataSigner->signingInfo.setSha256Signing();

    // Do not fetch publications older than 10 seconds
    ndn::svs::SVSPubSubOptions opts;
    opts.useTimestamp = false;
    // opts.maxPubAge = ndn::time::seconds(10);

    m_svsps = std::make_shared<ndn::svs::SVSPubSub>(
      ndn::Name(servicegroupName).append("NDNSD"),
      ndn::Name(nodeName),
      m_face,
      [](const std::vector<ndn::svs::MissingDataInfo>& data) {},
      opts,
      secOpts);

    std::string ndnsdUpdateRegex = "^(<>*)<NDNSD><service-info>";

    m_svsps->subscribeWithRegex(ndn::Regex(ndnsdUpdateRegex),
                                std::bind(&ServiceDiscovery::OnServiceUpdate, this, std::placeholders::_1),
                                true, false);

    std::string ndnsdDiscoveryRegex = "^(<>*)<NDNSD><discovery>";

    m_svsps->subscribeWithRegex(ndn::Regex(ndnsdDiscoveryRegex),
                                std::bind(&ServiceDiscovery::OnServiceDiscovery, this, std::placeholders::_1),
                                true, false);

    m_svsps->publish(ndn::Name().append(m_nodeName.toUri()).append("NDNSD").append("discovery").appendVersion(), ndn::span<const uint8_t>());
}
ServiceDiscovery::~ServiceDiscovery()
{
  stop();
}

void ServiceDiscovery::publishServiceDetail(Details details)
{
  NDN_LOG_DEBUG("Publishing service detail");
  m_serviceDetails[details.serviceName.toUri()] = details;
  ndn::Block block = details.encode();
  m_svsps->publish(ndn::Name().append(m_nodeName.toUri()).append(details.serviceName).append("NDNSD").append("service-info").appendVersion(), ndn::span<const uint8_t>(block.data(), block.size()));
}

void ServiceDiscovery::run()
{
  
}

void ServiceDiscovery::stop()
{
  
}

void ServiceDiscovery::OnServiceUpdate(const ndn::svs::SVSPubSub::SubscriptionData &subscription)
{
  NDN_LOG_DEBUG("Service update received : " << subscription.name);
  try
  {
    Details details = Details::decode(ndn::Block(subscription.data));
    m_receivedDetails[ndn::Name().append(details.applicationPrefix).append(details.serviceName).toUri()] = details;

    m_discoveryCallback(details);
  }
  catch (const std::exception& e)
  {
    NDN_LOG_DEBUG("Error decoding service detail: " << e.what());
    return;
  }
}

void ServiceDiscovery::OnServiceDiscovery(const ndn::svs::SVSPubSub::SubscriptionData &subscription)
{
  NDN_LOG_DEBUG("Discovery callback received : " << subscription.name);
  if (m_lastDiscoveryTime + ndn::time::seconds(5) > ndn::time::steady_clock::now()) {
    NDN_LOG_DEBUG("Skip discovery callback within 5 seconds");
    // Record the time, and won't do it in next 5 seconds
    m_lastDiscoveryTime = ndn::time::steady_clock::now();
    return;
  }
  // Record the time, and won't do it in next 5 seconds
  m_lastDiscoveryTime = ndn::time::steady_clock::now();
  // publish cached details
  for (auto& item : m_serviceDetails)
  {
    auto block = item.second.encode();
    m_svsps->publish(ndn::Name().append(m_nodeName.toUri()).append(item.first).append("NDNSD").append("service-info").appendVersion(), ndn::span<const uint8_t>(block.data(), block.size()));
  }
}

} // namespace discovery
} // namespace ndnsd
