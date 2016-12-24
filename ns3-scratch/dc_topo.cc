/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("FirstScriptExample");

#define NODES_IN_RACK 2
#define NUMBER_OF_RACKS 2

typedef struct {
  NodeContainer master;
  NodeContainer racks[NUMBER_OF_RACKS];
} dct;

NodeContainer create_rack(char *network, NodeContainer master);

dct create_dc() {
  dct dc;
  dc.master.Create(1);

  InternetStackHelper stack;
  stack.Install(dc.master);

  char network[20];
  int top = 1;
  int bottom = 1;
  NS_LOG_INFO("test1");
  for(int i = 0; i < NUMBER_OF_RACKS; i++) {
    sprintf(network, "10.%d.%d.0", top, bottom);
    dc.racks[i] = create_rack(network, dc.master);
    bottom++;
    if (bottom == 255) {
      top++;
      bottom = 1;
    }
  }

  PointToPointHelper p2p;
  NetDeviceContainer devices;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
  // about 100 meters of copper
  p2p.SetChannelAttribute("Delay", StringValue("500.0ns"));

  Ipv4AddressHelper address;
  address.SetBase("11.1.0.0", "255.255.0.0");

  for (int i = 0; i < NUMBER_OF_RACKS; i++) {
    NodeContainer master_and_router = NodeContainer(dc.master.Get(0), dc.racks[i].Get(0));
    devices = p2p.Install(master_and_router);

    address.Assign(devices);

    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll(ascii.CreateFileStream("dc_topo.tr"));
    p2p.EnablePcapAll("dc_topo");
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();
  
  return dc;
}

NodeContainer create_rack(char *network, NodeContainer master) {
  NodeContainer nodes;
  nodes.Create(1 + NODES_IN_RACK);

  InternetStackHelper stack;
  stack.Install(nodes);

  PointToPointHelper p2p;
  NetDeviceContainer devices;
  p2p.SetDeviceAttribute("DataRate", StringValue("10Gbps"));
  // about a meter of copper
  p2p.SetChannelAttribute("Delay", StringValue("5.0ns"));

  Ipv4AddressHelper address;
  address.SetBase(network, "255.255.255.255");

  for (int i = 1; i <= NODES_IN_RACK; i++) {
    NodeContainer router_and_node(nodes.Get(0), nodes.Get(i));
    devices = p2p.Install(router_and_node);
    address.Assign(devices);

    AsciiTraceHelper ascii;
    p2p.EnableAsciiAll(ascii.CreateFileStream("dc_topo.tr"));
    p2p.EnablePcapAll("dc_topo");
  }

  return nodes;
}

Ipv4Address get_addr(Ptr<Node> n, uint32_t ifn) {
  return n->GetObject<Ipv4>()->GetAddress(ifn + 1, 0).GetLocal();
}

int main(int argc, char *argv[]) {
  Time::SetResolution(Time::NS);
  LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);

  dct dc = create_dc();
  std::cout << "Master Router " << get_addr(dc.master.Get(0), 0) << std::endl;
  for(int i = 0; i < NUMBER_OF_RACKS; i++) {
    std::cout << "Rack " << i << " ToR Router " << get_addr(dc.racks[i].Get(0), 1) << std::endl;
  }
  std::cout << std::endl;
  for(int i = 0; i < NUMBER_OF_RACKS; i++) {
    std::cout << "Rack " << i << " ToR Router " << get_addr(dc.racks[i].Get(0), 0) << std::endl;
    for(int j = 1; j <= NODES_IN_RACK; j++) {
      std::cout << "Rack " << i << " Machine " << j << " " << get_addr(dc.racks[i].Get(j), 0) << std::endl;
    }
    std::cout << std::endl;
  }

  UdpEchoServerHelper echoServer(9);

  ApplicationContainer serverApps = echoServer.Install(dc.racks[0].Get(1));
  serverApps.Start(Seconds(1.0));
  serverApps.Stop(Seconds(10.0));


  UdpEchoClientHelper echoClient(get_addr(dc.racks[0].Get(1), 0), 9);
  echoClient.SetAttribute("MaxPackets", UintegerValue(1));
  echoClient.SetAttribute("Interval", TimeValue(Seconds (1.0)));
  echoClient.SetAttribute("PacketSize", UintegerValue(1024));

  ApplicationContainer clientApps = echoClient.Install(dc.racks[1].Get(2));
  clientApps.Start(Seconds(2.0));
  clientApps.Stop(Seconds(10.0));

  Simulator::Run();
  Simulator::Destroy();
  return 0;
}
