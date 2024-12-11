/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  The University of Memphis
 *
 * This file is originally from NLSR and is modified here per the use.
 * See NLSR's AUTHORS.md for complete list of NLSR authors and contributors.
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

  @ most part of sync-adapter code is taken from NLSR/communication/

 **/

#ifndef NDNSD_SYNC_ADAPTER_HPP
#define NDNSD_SYNC_ADAPTER_HPP

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>
#include <ChronoSync/logic.hpp>
#include <PSync/full-producer.hpp>

namespace ndnsd {

struct SyncDataInfo
{
  ndn::Name prefix;
  uint64_t highSeq;
  uint64_t lowSeq;
};

typedef std::function<void(const std::vector<SyncDataInfo>& updates)> SyncUpdateCallback;

enum {
  SYNC_PROTOCOL_CHRONOSYNC = 0,
  SYNC_PROTOCOL_PSYNC = 1
};

void 
printSyncUPdate(const std::vector<ndnsd::SyncDataInfo> updates);

class SyncProtocolAdapter
{
public:
  SyncProtocolAdapter(ndn::Face& facePtr,
                      uint8_t syncProtocol,
                      const ndn::Name& syncPrefix,
                      const ndn::Name& userPrefix,
                      ndn::time::milliseconds syncInterestLifetime,
                      const SyncUpdateCallback& syncUpdateCallback);

  /*! \brief Add user node to ChronoSync or PSync
   *
   * \param userPrefix the Name under which the application will publishData
   */
  void
  addUserNode(const ndn::Name& userPrefix);

  /*! \brief Publish update to ChronoSync or PSync
   *
   * NLSR forces sequences number on the sync protocol
   * as it manages is its own sequence number by storing it in a file.
   *
   * \param userPrefix the Name to be updated
   * \param seq the sequence of userPrefix
   */
  void
  publishUpdate(const ndn::Name& userPrefix);

// PUBLIC_WITH_TESTS_ELSE_PRIVATE:
   /*! \brief Hook function to call whenever ChronoSync detects new data.
   *
   * This function packages the sync information into discrete updates
   * and passes those off to another function, m_syncUpdateCallback.
   * \sa m_syncUpdateCallback
   *
   * \param v A container with the new information sync has received
   */
  void
  onChronoSyncUpdate(const std::vector<chronosync::MissingDataInfo>& updates);

   /*! \brief Hook function to call whenever PSync detects new data.
   *
   * This function packages the sync information into discrete updates
   * and passes those off to another function, m_syncUpdateCallback.
   * \sa m_syncUpdateCallback
   *
   * \param v A container with the new information sync has received
   */
  void
  onPSyncUpdate(const std::vector<psync::MissingDataInfo>& updates);

private:
  uint8_t m_syncProtocol;
  SyncUpdateCallback m_syncUpdateCallback;
  std::shared_ptr<chronosync::Logic> m_chronoSyncLogic;
  std::shared_ptr<psync::FullProducer> m_psyncLogic;
  ndn::KeyChain keyChain;
};

} // namespace ndnsd

#endif // NDNSD_SYNC_ADAPTER_HPP
