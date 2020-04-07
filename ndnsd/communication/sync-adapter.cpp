/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2019,  The University of Memphis
 *
 * This file is part of NDNSD.
 * See AUTHORS.md for complete list of NDNSD authors and contributors.
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

#include "sync-adapter.hpp"
#include <iostream>

INIT_LOGGER(SyncProtocolAdapter);

namespace ndnsd {

const auto FIXED_SESSION = ndn::name::Component::fromNumber(0);

SyncProtocolAdapter::SyncProtocolAdapter(ndn::Face& face,
                                         int32_t syncProtocol,
                                         const ndn::Name& syncPrefix,
                                         const ndn::Name& userPrefix,
                                         ndn::time::milliseconds syncInterestLifetime,
                                         const SyncUpdateCallback& syncUpdateCallback)
 : m_syncProtocol(syncProtocol)
 , m_syncUpdateCallback(syncUpdateCallback)
{
  if (m_syncProtocol == SYNC_PROTOCOL_CHRONOSYNC) {
    std::cout << "Using ChronoSync" << std::endl;
    // NDN_LOG_DEBUG("Using ChronoSync");
    m_chronoSyncLogic = std::make_shared<chronosync::Logic>(face,
                          syncPrefix,
                          userPrefix,
                          std::bind(&SyncProtocolAdapter::onChronoSyncUpdate, this, _1),
                          chronosync::Logic::DEFAULT_NAME,
                          chronosync::Logic::DEFAULT_VALIDATOR,
                          chronosync::Logic::DEFAULT_RESET_TIMER,
                          chronosync::Logic::DEFAULT_CANCEL_RESET_TIMER,
                          chronosync::Logic::DEFAULT_RESET_INTEREST_LIFETIME,
                          syncInterestLifetime,
                          chronosync::Logic::DEFAULT_SYNC_REPLY_FRESHNESS,
                          chronosync::Logic::DEFAULT_RECOVERY_INTEREST_LIFETIME,
                          FIXED_SESSION);
  }
  else {
    // NDN_LOG_DEBUG("Using PSync");
    std::cout << "Using PSync" << std::endl;
    m_psyncLogic = std::make_shared<psync::FullProducer>(80,
                     face,
                     syncPrefix,
                     userPrefix,
                     std::bind(&SyncProtocolAdapter::onPSyncUpdate, this, _1),
                     syncInterestLifetime);
  }
}

void
SyncProtocolAdapter::addUserNode(const ndn::Name& userPrefix)
{
  if (m_syncProtocol == SYNC_PROTOCOL_CHRONOSYNC) {
    m_chronoSyncLogic->addUserNode(userPrefix, chronosync::Logic::DEFAULT_NAME, FIXED_SESSION);
  }
  else {
    m_psyncLogic->addUserNode(userPrefix);
  }
}

void
SyncProtocolAdapter::publishUpdate(const ndn::Name& userPrefix)
{
  std::cout << "Publishing update for Sync Prefix" << userPrefix << std::endl;
  if (m_syncProtocol == SYNC_PROTOCOL_CHRONOSYNC) {
    m_chronoSyncLogic->updateSeqNo(m_chronoSyncLogic->getSeqNo(userPrefix), userPrefix);
  }
  else {
    m_psyncLogic->publishName(userPrefix);
  }
}

void
SyncProtocolAdapter::onChronoSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates)
{
  std::cout << "Received ChronoSync update event" << std::endl;
  std::vector<ndnsd::SyncDataInfo> dinfo;

  for (const auto& update : updates) {
    // Remove FIXED_SESSION
    SyncDataInfo di;
    di.prefix = update.session.getPrefix(-1);
    di.highSeq = update.high;
    dinfo.insert(dinfo.begin(), di);
  }
  m_syncUpdateCallback(dinfo);
}

void
SyncProtocolAdapter::onPSyncUpdate(const std::vector<psync::MissingDataInfo>& updates)
{
  std::cout << "Received PSync update event" << std::endl;
  std::vector<ndnsd::SyncDataInfo> dinfo;

  for (const auto& update : updates) {
    // Remove FIXED_SESSION
    SyncDataInfo di;
    di.prefix = update.prefix;
    di.highSeq = update.highSeq;
    dinfo.insert(dinfo.begin(), di);
  }
  m_syncUpdateCallback(dinfo);
}

} // namespace ndnsd