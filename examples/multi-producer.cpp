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
#include <vector>

NDN_LOG_INIT(ndnsd.examples.MultiProducerApp);

inline bool
isFile(const std::string& fileName)
{
  return boost::filesystem::exists(fileName);
}

void
processCallback(const ndnsd::discovery::Reply& callback)
{
    NDN_LOG_INFO("Service publish callback received");
    auto status = (callback.status == ndnsd::discovery::ACTIVE)? "ACTIVE": "EXPIRED";
    NDN_LOG_INFO("Status: " << status);
    for (auto& item : callback.serviceDetails)
    {
        NDN_LOG_INFO("Callback: " << item.first << ":" << item.second);
    }
}

int main(int argc, char* argv[]) {
  int syncProtocol = ndnsd::SYNC_PROTOCOL_CHRONOSYNC;

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <file1> <file2> ..." << std::endl;
    return 1;
  }

  // std::map<char, uint8_t> flags;
  // flags.insert(std::pair<char, uint8_t>('p', syncProtocol)); // Protocol choice
  // flags.insert(std::pair<char, uint8_t>('t', ndnsd::discovery::PRODUCER)); // Type producer: 1

  std::map<char, uint8_t> flagsMap = {
    {'p', ndnsd::SYNC_PROTOCOL_CHRONOSYNC}, // Protocol choice
    {'t', ndnsd::discovery::PRODUCER}  // Type consumer: 0
  };

  ndnsd::discovery::MultiServiceDiscovery multiServiceDiscovery;

  try {
    for (int i = 1; i < argc; ++i) {
      if (!isFile(argv[i])) {
        std::cerr << "File does not exist: " << argv[i] << std::endl;
        return 1;
      }
      auto serviceDiscovery = std::make_shared<ndnsd::discovery::ServiceDiscovery>(std::string(argv[i]), flagsMap, processCallback);
      multiServiceDiscovery.addServiceDiscovery(serviceDiscovery);
    }

    NDN_LOG_INFO("Starting multi-service discovery");
    multiServiceDiscovery.startAll();
    sleep(30);
    multiServiceDiscovery.stopAll();
  }
  catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    NDN_LOG_ERROR("Cannot start multi-service discovery: " << e.what());
    return 1;
  }

  return 0;
}
