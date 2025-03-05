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

#include "ndnsd/discovery/service-discovery.hpp"
#include <ndn-cxx/util/logger.hpp>

#include <iostream>
#include <cstdlib>

// #include <list>

NDN_LOG_INIT(ndnsd.examples.ProducerApp);

inline bool
isFile(const std::string& fileName)
{
  return boost::filesystem::exists(fileName);
}

class Producer
{
public:

  Producer(ndn::Face& face, const std::string& syncGroupName, const std::string& nodeName)
    : m_face(face),
      m_syncGroupName(syncGroupName),
      m_nodeName(nodeName),
      m_serviceDiscovery(syncGroupName, nodeName, m_face, m_keyChain, std::bind(&Producer::processCallback, this, _1))
  {
    execute ();
  }
  void
  execute ()
  {
    NDN_LOG_DEBUG("Executing producer");
    ndnsd::discovery::Details details_0{ndn::Name("/FlightControl/Takeoff"),
                                      ndn::Name(m_nodeName),
                                      3600,
                                      time(NULL),
                                      {{"type", "flight control"}, {"version", "1.0.0"}, {"tokenName", m_nodeName+"/NDNSF/TOKEN/FlightControl/Takeoff"}}};
                                    
    ndnsd::discovery::Details details_1{ndn::Name("/FlightControl/Land"),
                                      ndn::Name(m_nodeName),
                                      3600,
                                      time(NULL),
                                      {{"type", "flight control"}, {"version", "1.0.0"}, {"tokenName", m_nodeName+"/NDNSF/TOKEN/FlightControl/Land"}}};
    m_serviceDiscovery.publishServiceDetail(details_0);
    m_serviceDiscovery.publishServiceDetail(details_1);
  }

private:
  void
  processCallback(const ndnsd::discovery::Details& details)
  {
    NDN_LOG_INFO("Service publish callback received");
    for(const auto& detail : m_serviceDiscovery.getReceivedServiceDetails()){
      NDN_LOG_INFO(detail.second.toString());
    }
  }

private:
  ndn::Face& m_face;
  ndn::KeyChain m_keyChain;
  std::string m_syncGroupName;
  std::string m_nodeName;
  ndnsd::discovery::ServiceDiscovery m_serviceDiscovery;
};

int
main(int argc, char* argv[])
{
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <syncGroupName> <nodeName>" << std::endl;
    return 1;
  }

  try {
    NDN_LOG_INFO("Starting producer application");
    ndn::Face face;
    std::thread m_thread([&face](){face.processEvents(ndn::time::seconds(100),true);});
    Producer producer(face, argv[1], argv[2]);
    m_thread.join();
  }
  catch (const std::exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    NDN_LOG_ERROR("Cannot execute producer, try again later: " << e.what());
  }
}
