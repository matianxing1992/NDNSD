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
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

NDN_LOG_INIT(ndnsd.examples.MultiConsumerApp);

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
  namespace po = boost::program_options;
  std::vector<std::string> serviceNames;
  int syncProtocol = ndnsd::SYNC_PROTOCOL_CHRONOSYNC;

  po::options_description desc("Options");
  desc.add_options()
      ("help,h", "Print help message")
      ("service-name,s", po::value<std::vector<std::string>>(&serviceNames)->multitoken()->required(), "Service names to fetch service info")
      ("sync-protocol,p", po::value<int>(&syncProtocol), "Sync protocol choice, 1 for psync, 0 for chronosync")
  ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    po::notify(vm);
  } catch (const po::error& e) {
    std::cerr << "Error parsing command line: " << e.what() << std::endl;
    return 1;
  }

  // std::map<char, uint8_t> flags;
  // flags.insert(std::pair<char, uint8_t>('p', syncProtocol)); // Protocol choice
  // flags.insert(std::pair<char, uint8_t>('t', ndnsd::discovery::CONSUMER)); // Type consumer: 0

  std::map<char, uint8_t> flagsMap = {
    {'p', ndnsd::SYNC_PROTOCOL_CHRONOSYNC}, // Protocol choice
    {'t', ndnsd::discovery::CONSUMER}  // Type consumer: 0
  };

  ndnsd::discovery::MultiServiceDiscovery multiServiceDiscovery;

  try {
    for (const auto& serviceName : serviceNames) {
      auto serviceDiscovery = std::make_shared<ndnsd::discovery::ServiceDiscovery>(ndn::Name(serviceName), flagsMap, processCallback);
      multiServiceDiscovery.addServiceDiscovery(serviceDiscovery);
    }

    multiServiceDiscovery.startAll();
    sleep(30);
    multiServiceDiscovery.stopAll();
  }
  catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
