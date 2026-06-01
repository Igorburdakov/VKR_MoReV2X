/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * HIGHWAY_fcd.cc — MoReV2X scenario with external ns-2 mobility trace.
 *
 * Based on HIGHWAY.cc, but replaces ConstantVelocityMobilityModel with
 * Ns2MobilityHelper to feed SUMO FCD-derived traces.
 *
 * Key new parameters compared to HIGHWAY.cc:
 *   --mobilityTrace  Path to ns-2 .tcl mobility trace (from fcd_to_ns2.py)
 *   --pKeep          Resource keep probability (slProbResourceKeep, default 0.0)
 *   --RRI            Resource Reservation Interval in ms (default 100)
 *   --CAMInterval    CAM packet generation period in ms (default = RRI).
 *                    Decouple from RRI to calibrate channel load (CBR) while
 *                    keeping the SPS configuration constant. ETSI EN 302 637-2
 *                    default ≈ 100 ms (10 Hz).
 *   --CBRxMin/Max    UE x-coordinate window (m) for CBR sampling. Defaults
 *                    are wide-open (±1e9), so every scenario gets CBR samples.
 *                    To reproduce the legacy 3GPP highway-straight gate use
 *                    --CBRxMin=1500 --CBRxMax=3500.
 *   --SavingPeriod   Period (s) at which CBR/Tx logs are flushed to disk
 *                    (default 1.0 s). Smaller → finer time resolution.
 *   --OutputPath     Directory for log files (default auto-generated)
 *
 * Usage example (exp01: µ=0, MCS=13, P=0.4, RRI=100, txPower=23):
 *   ./waf --run "HIGHWAY_fcd \
 *     --mobilityTrace=/path/to/sumo_ns2_trace.tcl \
 *     --Vehicles=20 --mcs=13 --Numerology=0 --ChannelBW=10 --SubChannel=50 \
 *     --ueTxPower=23 --pKeep=0.4 --RRI=100 --simTime=100 \
 *     --OutputPath=results/exp01_fcd/"
 */

#include <ns3/applications-module.h>
#include <ns3/config-store.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/network-module.h>
#include <ns3/nist-lte-helper.h>
#include <ns3/MoReV2X-module.h>
#include "ns3/nist-sl-resource-pool-factory.h"
#include "ns3/nist-sl-preconfig-pool-factory.h"
#include "ns3/delay-jitter-estimation.h"
#include "ns3/packet.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include "ns3/packet-tag-list.h"
#include <ns3/multi-model-spectrum-channel.h>
#include "ns3/random-variable-stream.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include "ns3/buildings-helper.h"
#include "ns3/nr-v2x-propagation-loss-model.h"
#include <cmath>
#include "ns3/rng-seed-manager.h"
#include "ns3/nr-v2x-tag.h"
#include "ns3/nr-v2x-utils.h"
#include <random>
#include <ns3/nr-v2x-amc.h>
#include "ns3/ns2-mobility-helper.h"

NS_LOG_COMPONENT_DEFINE ("HIGHWAY_fcd");

using namespace ns3;

// =========== Global state (same as HIGHWAY.cc) ===================
uint64_t packetID = 0;
double simTime;
Ipv4Address groupAddress;

int TrepPrint = 100; // ms

std::vector<uint16_t> PeriodicPKTs_Size;
uint16_t LargestPeriodicSize;

std::vector<double> Periodic_Tgen;
std::vector<double> PDB_Periodic;
std::vector<double> PrevX, PrevY, PrevZ, VelX, VelY, VelZ;
std::vector<bool> EnableTX;
std::vector<uint8_t> VehicleTrafficType;
std::vector<uint16_t> Pattern_index;

double MeasInterval;
std::string FilePath;

PosEnabler PositionChecker;

// =========== UdpClient::Send override (periodic traffic only) ====
void
UdpClient::Send (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_sendEvent.IsExpired ());

  Ptr<Node> currentNode = GetNode ();
  uint32_t nodeId = currentNode->GetId ();

  if (EnableTX[nodeId - 1])
    {
      SeqTsHeader seqTs;
      seqTs.SetSeq (m_sent);

      NrV2XTag v2xTag;
      v2xTag.SetGenTime (Simulator::Now ().GetSeconds ());
      v2xTag.SetMessageType (0x00);
      v2xTag.SetTrafficType (VehicleTrafficType[nodeId - 1]);
      v2xTag.SetPPPP (0x00);
      // PATCH 2026-05-28: removed duplicate SetPrsvp(m_interval) below.
      // PRSVP (SCI-1 field, see 3GPP TS 38.212 §8.4.1.1) must equal the
      // SPS reservation interval RRI, which is stored in PDB_Periodic.
      // CAM-generation period (Periodic_Tgen) is purely application-level
      // and never reaches MAC — it only controls UdpClient re-scheduling.
      v2xTag.SetNodeId ((uint32_t)nodeId);
      v2xTag.SetReselectionCounter ((uint16_t)10000);

      double T_gen = Periodic_Tgen[nodeId - 1];
      v2xTag.SetPrsvp ((double)PDB_Periodic[nodeId - 1]); // = RRI
      v2xTag.SetPdb ((double)PDB_Periodic[nodeId - 1]);
      uint32_t m_size = PeriodicPKTs_Size[Pattern_index[nodeId - 1] % 5];
      v2xTag.SetPacketSize ((uint16_t)m_size + 35);
      v2xTag.SetReservationSize ((uint16_t)m_size + 35);

      packetID++;
      v2xTag.SetIntValue (packetID);
      v2xTag.SetPacketId (packetID);
      v2xTag.SetDoubleValue (Simulator::Now ().GetSeconds ());

      Ptr<MobilityModel> mobility = GetNode ()->GetObject<MobilityModel> ();
      Vector currentPos = mobility->GetPosition ();
      v2xTag.SetGenPosX (currentPos.x);
      v2xTag.SetGenPosY (currentPos.y);
      v2xTag.SetNumHops (0);

      Ptr<Packet> p = Create<Packet> (m_size - (8 + 4));
      p->AddHeader (seqTs);
      p->AddByteTag (v2xTag);

      TypeId UDPtid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      Ptr<Socket> sendSocket = Socket::CreateSocket (currentNode, UDPtid);
      sendSocket->SetAllowBroadcast (true);
      sendSocket->Bind ();
      sendSocket->Connect (InetSocketAddress (groupAddress, 8000));
      if ((sendSocket->Send (p)) >= 0)
        {
          ++m_sent;
          NS_LOG_INFO ("TX " << p->GetSize () << " B, node " << nodeId
                             << " t=" << Simulator::Now ().GetSeconds ());
        }

      if (m_sent < m_count)
        {
          Pattern_index[nodeId - 1]++;
          m_sendEvent = Simulator::Schedule (MilliSeconds (T_gen), &UdpClient::Send, this);
        }
    }
  else
    {
      m_sendEvent = Simulator::Schedule (MilliSeconds (TrepPrint), &UdpClient::Send, this);
    }
}

// =========== PacketSink::HandleRead override =====================
void
PacketSink::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Ptr<Node> currentNode = GetNode ();
  uint32_t nodeId = currentNode->GetId ();

  NrV2XTag rxV2xTag;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        break;
      m_totalRx += packet->GetSize ();
      if (packet->FindFirstMatchingByteTag (rxV2xTag))
        {
          uint64_t rxPacketID = rxV2xTag.GetIntValue ();
          Ptr<LTENodeState> nodeState = currentNode->GetObject<LTENodeState> ();
          nodeState->SetLastRcvPacketId (rxPacketID);
          NS_LOG_UNCOND ("RX node " << nodeId << " pkt " << rxPacketID);
        }
      SeqTsHeader seqTs;
      packet->RemoveHeader (seqTs);
    }
}

// =========== Periodic position logger ============================
void
Print (NodeContainer VehicleUEs)
{
  std::ofstream positFile;
  positFile.open (FilePath + "posFile.txt", std::ofstream::app);
  for (NodeContainer::Iterator L = VehicleUEs.Begin (); L != VehicleUEs.End (); ++L)
    {
      Ptr<Node> node = *L;
      uint32_t ID = node->GetId ();
      Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
      if (!mob)
        continue;
      Vector pos = mob->GetPosition ();

      VelX[ID - 1] = (pos.x - PrevX[ID - 1]) / MeasInterval;
      VelY[ID - 1] = (pos.y - PrevY[ID - 1]) / MeasInterval;
      VelZ[ID - 1] = (pos.z - PrevZ[ID - 1]) / MeasInterval;

      positFile << Simulator::Now ().GetSeconds () << "," << ID << "," << pos.x << "," << pos.y
                << "," << pos.z << "," << VelX[ID - 1] << "," << VelY[ID - 1] << ","
                << VelZ[ID - 1] << "," << (int)VehicleTrafficType[ID - 1] << ","
                << "1\r\n";

      EnableTX[ID - 1] = true;
      if (!PositionChecker.isEnabled (pos))
        EnableTX[ID - 1] = false;

      PrevX[ID - 1] = pos.x;
      PrevY[ID - 1] = pos.y;
      PrevZ[ID - 1] = pos.z;
    }
  positFile.close ();
  Simulator::Schedule (MilliSeconds (TrepPrint), &Print, VehicleUEs);
}

// =========== main ================================================
int
main (int argc, char *argv[])
{
  auto __ph_t0 = std::chrono::steady_clock::now();
  auto __ph_prev = __ph_t0;
  auto __ph_mark = [&](const char *tag){
    auto now = std::chrono::steady_clock::now();
    double abs_s = std::chrono::duration<double>(now-__ph_t0).count();
    double dt_s  = std::chrono::duration<double>(now-__ph_prev).count();
    __ph_prev = now;
    std::cerr << "[PHASE-MV2X] " << tag << " abs=" << abs_s << "s delta=" << dt_s << "s\n";
    std::cerr.flush();
  };
  __ph_mark("start");
  // ------ parameters -----------------------------------------------
  uint32_t mcs = 13;
  uint32_t pscchLength = 8;
  std::string period = "sf40";
  simTime = 100.0;
  double ueTxPower = 23.0;
  uint32_t ueCount = 20;
  bool verbose = true;
  uint16_t OFDM_numerology = 0;
  uint16_t channelBW = 10; // MHz
  uint16_t channelBW_RBs;
  uint32_t subchannelSize = 50;
  uint32_t highwayLength = 5000; // kept for PositionChecker init (disabled)

  std::string mobilityTrace = ""; // required: path to ns-2 .tcl trace
  double pKeep = 0.0;             // resource keep probability
  uint16_t RRI = 100;             // resource reservation interval [ms]
  // PATCH 2026-05-28: CAM-generation interval is decoupled from RRI to allow
  // CBR calibration. Default = RRI (legacy behaviour). Set explicitly via
  // --CAMInterval=<ms> to vary channel load independently of SPS.
  uint16_t CAMInterval = 0;       // 0 → fallback to RRI (set after parse)
  // PATCH 2026-05-28: configurable CBR x-window (wide-open by default).
  double cbrXMin = -1e9;
  double cbrXMax =  1e9;
  // PATCH 2026-05-28: log flush period (s). 2.0 s was legacy; 1.0 s gives
  // finer CBR time-resolution. Keep ≥ 2× m_CBRCheckingPeriod (=0.5 s).
  double savingPeriod = 1.0;
  std::string outputPath = "";    // auto-generated if empty

  // HARQ (blind sidelink retransmissions). Maps to NrV2XUeMac::EnableReTx.
  //   harq=1 → EnableReTx=false (1 transmission per TB, baseline AoI study)
  //   harq=2 → EnableReTx=true  (1 NDI + 1 blind retransmission, see
  //            NrV2XUeMac::UnimoreSortSelections in nr-v2x-ue-mac.cc:1585)
  //   harq>2 → NOT natively supported by current NrV2XUeMac SB-SPS path —
  //            UnimoreSortSelections orders exactly 2 selections per RRI set.
  uint16_t harq = 1;

  uint32_t seed = 867;
  uint32_t runNumber = 1;

  std::map<uint16_t, uint16_t> SCS_factor = {{0, 1}, {1, 2}, {2, 4}, {3, 8}};

  // PositionChecker polygons (disabled below — all vehicles always transmit)
  Point polygonTX[] = {{-99999, -99999}, {99999, -99999}, {99999, 99999}, {-99999, 99999}};
  Point polygonRX[] = {{-99999, -99999}, {99999, -99999}, {99999, 99999}, {-99999, 99999}};

  CommandLine cmd;
  cmd.AddValue ("Vehicles", "Number of vehicles (must match trace)", ueCount);
  cmd.AddValue ("mcs", "MCS", mcs);
  cmd.AddValue ("ueTxPower", "TX Power [dBm]", ueTxPower);
  cmd.AddValue ("simTime", "Simulation time [s]", simTime);
  cmd.AddValue ("Numerology", "OFDM numerology index (0/1/2)", OFDM_numerology);
  cmd.AddValue ("ChannelBW", "Channel bandwidth [MHz]", channelBW);
  cmd.AddValue ("SubChannel", "Subchannel size [RBs]", subchannelSize);
  cmd.AddValue ("mobilityTrace", "Path to ns-2 .tcl mobility trace from SUMO FCD", mobilityTrace);
  cmd.AddValue ("pKeep", "Resource keep probability (slProbResourceKeep)", pKeep);
  cmd.AddValue ("RRI", "Resource Reservation Interval [ms]", RRI);
  cmd.AddValue ("CAMInterval", "CAM generation period [ms]; default=RRI", CAMInterval);
  cmd.AddValue ("CBRxMin", "Lower x-bound (m) for CBR sampling", cbrXMin);
  cmd.AddValue ("CBRxMax", "Upper x-bound (m) for CBR sampling", cbrXMax);
  cmd.AddValue ("SavingPeriod", "Period (s) to flush CBR/Tx logs", savingPeriod);
  cmd.AddValue ("OutputPath", "Output directory for log files", outputPath);
  cmd.AddValue ("harq", "Max transmissions per TB on PSSCH (1 or 2; >2 not supported)", harq);
  cmd.AddValue ("seed", "Random seed", seed);
  cmd.AddValue ("runNo", "Run number", runNumber);
  cmd.AddValue ("verbose", "Verbose logging", verbose);
  cmd.Parse (argc, argv);

  // PATCH 2026-05-28: default CAMInterval = RRI if user did not override.
  if (CAMInterval == 0)
    CAMInterval = RRI;

  NS_ABORT_MSG_IF (harq < 1 || harq > 2,
                   "HIGHWAY_fcd: --harq must be 1 or 2 (NrV2XUeMac SB-SPS "
                   "does not natively support more than 2 transmissions per TB; "
                   "harq>=3 must be skipped in the MoReV2X pipeline)");

  NS_ABORT_MSG_IF (mobilityTrace.empty (), "You must specify --mobilityTrace=<path_to_.tcl>");
  NS_ABORT_MSG_IF (SCS_factor.find (OFDM_numerology) == SCS_factor.end (),
                   "Invalid OFDM numerology (0/1/2 supported)");

  channelBW_RBs = GetRbsFromBW (15 * SCS_factor[OFDM_numerology], channelBW);
  NS_ASSERT_MSG (subchannelSize > 0, "Subchannel size must be > 0");
  NS_ASSERT_MSG (channelBW_RBs >= subchannelSize, "BW must be >= subchannel size");

  NS_LOG_UNCOND ("HIGHWAY_fcd: µ=" << OFDM_numerology << " MCS=" << mcs
                                    << " pKeep=" << pKeep << " RRI=" << RRI
                                    << " txPower=" << ueTxPower << " Vehicles=" << ueCount
                                    << " SubCh=" << subchannelSize << " BWrb=" << channelBW_RBs);

  // ------ output path ----------------------------------------------
  if (outputPath.empty ())
    outputPath = "results/fcd_Periodic_mu" + std::to_string (OFDM_numerology) + "_mcs" +
                 std::to_string (mcs) + "_pk" + std::to_string ((int)(pKeep * 10)) + "_rri" +
                 std::to_string (RRI) + "_tx" + std::to_string ((int)ueTxPower) + "_" +
                 std::to_string (seed) + "_" + std::to_string (runNumber) + "/";

  FilePath = outputPath;
  system (("rm -rf " + outputPath).c_str ());
  system (("mkdir -p " + outputPath).c_str ());

  // ------ simREADME ------------------------------------------------
  {
    std::ofstream readme;
    readme.open (outputPath + "simREADME.txt");
    readme << "----------------------\n";
    readme << "Simulation (HIGHWAY_fcd — MoReV2X + SUMO FCD mobility):\n";
    readme << " - simulation time = " << simTime << " s\n";
    readme << " - seed = " << seed << "\n";
    readme << " - run = " << runNumber << "\n";
    readme << " - UE count = " << ueCount << "\n";
    readme << " - MCS = " << mcs << "\n";
    readme << " - OFDM numerology index = " << OFDM_numerology << "\n";
    readme << " - SubChannel size = " << subchannelSize << " RBs\n";
    readme << " - Channel BW = " << channelBW << " MHz (" << channelBW_RBs << " RBs)\n";
    readme << " - TX power = " << ueTxPower << " dBm\n";
    readme << " - pKeep (KeepProbability) = " << pKeep << "\n";
    readme << " - RRI = " << RRI << " ms (SPS reservation interval)\n";
    readme << " - CAMInterval = " << CAMInterval << " ms (CAM generation period)\n";
    readme << " - CBR x-window = [" << cbrXMin << ", " << cbrXMax << "] m\n";
    readme << " - SavingPeriod = " << savingPeriod << " s\n";
    readme << " - Mobility trace = " << mobilityTrace << "\n";
    readme << " - HARQ (TxPerTB)  = " << harq
           << "  (EnableReTx=" << (harq == 2 ? "true" : "false") << ")\n";
    readme << "----------------------\n";
    readme.close ();
  }

  // ------ random seed ----------------------------------------------
  RngSeedManager::SetSeed (seed);
  RngSeedManager::SetRun (runNumber);

  // ------ PositionChecker (disabled — all UEs transmit) -----------
  PositionChecker.initPolygon (polygonTX, (int)sizeof (polygonTX) / sizeof (polygonTX[0]), "TX");
  PositionChecker.initPolygon (polygonRX, (int)sizeof (polygonRX) / sizeof (polygonRX[0]), "RX");
  PositionChecker.DisableChecker ();

  // ------ PHY / MAC defaults ---------------------------------------
  double RefSensitivity = -103.5;
  double CBR_RSSIthreshold = -88.0;

  Config::SetDefault ("ns3::NrV2XUeMac::SlGrantMcs", UintegerValue (mcs));
  Config::SetDefault ("ns3::NrV2XUeMac::ListL2Enabled", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::MixedTraffic", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::AllowReEvaluation", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::AllSlotsReEvaluation", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::RSRPthreshold", DoubleValue (-128.0));
  Config::SetDefault ("ns3::NrV2XUeMac::EnableReTx", BooleanValue (harq == 2));
  Config::SetDefault ("ns3::NrV2XUeMac::DynamicScheduling", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::AdaptiveScheduling", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::UMHReEvaluation", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::FrequencyReuse", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XUeMac::RandomV2VSelection", BooleanValue (false));
  // *** pKeep attribute added to NrV2XUeMac::GetTypeId() ***
  Config::SetDefault ("ns3::NrV2XUeMac::KeepProbability", DoubleValue (pKeep));
  Config::SetDefault ("ns3::NrV2XUeMac::SubchannelSize", UintegerValue (subchannelSize));
  Config::SetDefault ("ns3::NrV2XUeMac::RBsBandwidth", UintegerValue (channelBW_RBs));
  Config::SetDefault ("ns3::NrV2XUeMac::SlotDuration",
                      DoubleValue (1.0 / SCS_factor[OFDM_numerology]));
  Config::SetDefault ("ns3::NrV2XUeMac::NumerologyIndex", UintegerValue (OFDM_numerology));

  Config::SetDefault ("ns3::NrV2XUePhy::TxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::Pcmax", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::PoNominalPusch", IntegerValue (-106));
  Config::SetDefault ("ns3::NistLteUePowerControl::PscchTxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NistLteUePowerControl::PsschTxPower", DoubleValue (ueTxPower));
  Config::SetDefault ("ns3::NrV2XUePhy::RsrpUeMeasThreshold", DoubleValue (-10.0));
  Config::SetDefault ("ns3::NrV2XUePhy::ReferenceSensitivity", DoubleValue (RefSensitivity));
  Config::SetDefault ("ns3::NrV2XUePhy::RSSIthreshold", DoubleValue (CBR_RSSIthreshold));

  Config::SetDefault ("ns3::NrV2XSpectrumPhy::ReferenceSensitivity", DoubleValue (RefSensitivity));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::CtrlErrorModelEnabled", BooleanValue (true));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::CtrlFullDuplexEnabled", BooleanValue (false));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SaveCollisionLossesUnimore", BooleanValue (true));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubchannelSize", UintegerValue (subchannelSize));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::RBsBandwidth", UintegerValue (channelBW_RBs));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SlotDuration",
                      DoubleValue (1.0 / SCS_factor[OFDM_numerology]));

  switch (OFDM_numerology)
    {
    case 0:
      Config::SetDefault ("ns3::NrV2XUePhy::SidelinkDataDuration",
                          TimeValue (NanoSeconds (1e6 - 71350 - 1)));
      Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubCarrierSpacing", UintegerValue (15));
      Config::SetDefault ("ns3::NrV2XUePhy::SubCarrierSpacing", UintegerValue (15));
      break;
    case 1:
      Config::SetDefault ("ns3::NrV2XUePhy::SidelinkDataDuration",
                          TimeValue (NanoSeconds (0.5e6 - 35680 - 1)));
      Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubCarrierSpacing", UintegerValue (30));
      Config::SetDefault ("ns3::NrV2XUePhy::SubCarrierSpacing", UintegerValue (30));
      break;
    case 2:
      Config::SetDefault ("ns3::NrV2XUePhy::SidelinkDataDuration",
                          TimeValue (NanoSeconds (0.25e6 - 17840 - 1)));
      Config::SetDefault ("ns3::NrV2XSpectrumPhy::SubCarrierSpacing", UintegerValue (60));
      Config::SetDefault ("ns3::NrV2XUePhy::SubCarrierSpacing", UintegerValue (60));
      break;
    default:
      NS_ABORT_MSG ("Unsupported numerology");
    }

  Config::SetDefault ("ns3::NrV2XUePhy::SlotDuration",
                      DoubleValue (1.0 / SCS_factor[OFDM_numerology]));
  Config::SetDefault ("ns3::NrV2XUePhy::SubchannelSize", UintegerValue (subchannelSize));
  Config::SetDefault ("ns3::NrV2XUePhy::RBsBandwidth", UintegerValue (channelBW_RBs));
  Config::SetDefault ("ns3::NrV2XUePhy::IBE", BooleanValue (true));
  Config::SetDefault ("ns3::NistLtePhy::TTI",
                      DoubleValue (1.0 / (1000 * SCS_factor[OFDM_numerology])));

  Config::SetDefault ("ns3::NistLteRlcUm::MaxTxBufferSize", StringValue ("100000"));
  Config::SetDefault ("ns3::DropTailQueue::MaxPackets", UintegerValue (10000000));

  Config::SetDefault ("ns3::NrV2XPropagationLossModel::Frequency", DoubleValue (5.9));
  Config::SetDefault ("ns3::NrV2XPropagationLossModel::Sigma", DoubleValue (3.0));
  Config::SetDefault ("ns3::NrV2XPropagationLossModel::SigmaNLOSv", DoubleValue (4.0));
  Config::SetDefault ("ns3::NrV2XPropagationLossModel::DecorrDistance", DoubleValue (25.0));

  // PATCH 2026-05-28: SavingPeriod is now driven by CLI (default 1.0 s).
  double SavingPeriod = savingPeriod;
  // PATCH 2026-05-28: configurable CBR evaluation window via Attributes.
  Config::SetDefault ("ns3::NrV2XUePhy::CBRxMin", DoubleValue (cbrXMin));
  Config::SetDefault ("ns3::NrV2XUePhy::CBRxMax", DoubleValue (cbrXMax));
  Config::SetDefault ("ns3::NistLteRlcUm::OutputPath", StringValue (outputPath));
  Config::SetDefault ("ns3::NrV2XUeMac::OutputPath", StringValue (outputPath));
  Config::SetDefault ("ns3::NrV2XUePhy::OutputPath", StringValue (outputPath));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::OutputPath", StringValue (outputPath));
  Config::SetDefault ("ns3::NrV2XUeMac::SavingPeriod", DoubleValue (SavingPeriod));
  Config::SetDefault ("ns3::NrV2XUePhy::SavingPeriod", DoubleValue (SavingPeriod));
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::SavingPeriod", DoubleValue (SavingPeriod));

  // ------ helpers --------------------------------------------------
  Ptr<NistPointToPointEpcHelper> epcHelper = CreateObject<NistPointToPointEpcHelper> ();
  Ptr<NistLteHelper> lteHelper = CreateObject<NistLteHelper> ();
  lteHelper->SetPathlossModelType ("ns3::NrV2XPropagationLossModel");

  Ptr<NrV2XPropagationLossModel> Sl3GPPChannelMatrix =
      CreateObject<NrV2XPropagationLossModel> ();
  Config::SetDefault ("ns3::NrV2XSpectrumPhy::ChannelMatrix",
                      PointerValue (Sl3GPPChannelMatrix));

  lteHelper->SetAttribute ("UseSidelink", BooleanValue (true));
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->Initialize ();

  Ptr<NistLteProseHelper> proseHelper = CreateObject<NistLteProseHelper> ();
  proseHelper->SetLteHelper (lteHelper);

  // ------ create UE nodes ------------------------------------------
  NodeContainer ueResponders;
  for (uint32_t t = 0; t < ueCount; ++t)
    {
      Ptr<Node> lteNode = CreateObject<Node> ();
      Ptr<LTENodeState> nodeState = CreateObject<LTENodeState> ();
      nodeState->SetNode (lteNode);
      lteNode->AggregateObject (nodeState);
      ueResponders.Add (lteNode);
    }

  // ------ MOBILITY: ns-2 trace from SUMO FCD ----------------------
  {
    // Place all nodes at a default "inactive" position first.
    MobilityHelper mobilityInit;
    Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator> ();
    for (uint32_t i = 0; i < ueCount; i++)
      posAlloc->Add (Vector (-10000.0 + i * 10, -10000.0, 0));
    mobilityInit.SetPositionAllocator (posAlloc);
    mobilityInit.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
    mobilityInit.Install (ueResponders);
  }

  NS_LOG_UNCOND ("Loading ns-2 mobility trace: " << mobilityTrace);
  Ns2MobilityHelper ns2mobility (mobilityTrace);
  ns2mobility.Install (ueResponders.Begin (), ueResponders.End ());

  // ------ Initialize per-node state vectors -----------------------
  for (NodeContainer::Iterator L = ueResponders.Begin (); L != ueResponders.End (); ++L)
    {
      Ptr<Node> node = *L;
      Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
      Vector pos = mob ? mob->GetPosition () : Vector (0, 0, 0);

      PrevX.push_back (pos.x);
      PrevY.push_back (pos.y);
      PrevZ.push_back (pos.z);
      VelX.push_back (0);
      VelY.push_back (0);
      VelZ.push_back (0);
      VehicleTrafficType.push_back (0x00); // periodic
      EnableTX.push_back (true);
    }

  Sl3GPPChannelMatrix->InitChannelMatrix (ueResponders);

  // ------ traffic model (periodic) --------------------------------
  {
    for (NodeContainer::Iterator L = ueResponders.Begin (); L != ueResponders.End (); ++L)
      {
        Ptr<Node> node = *L;
        uint32_t ID = node->GetId ();
        // PATCH 2026-05-28: PDB is sized by RRI (SPS reservation), but the
        // generator inter-arrival time uses CAMInterval. Decoupling allows
        // CBR calibration: vary CAMInterval to change channel load without
        // touching SPS. m_maxPDB=250 ms ceiling (see nr-v2x-ue-mac.cc patch).
        PDB_Periodic.push_back ((double)RRI);
        Periodic_Tgen.push_back ((double)CAMInterval);
        (void)ID; // suppress unused warning
      }

    // Packet size: 200 B (account for 35 B overhead → 165 B payload + 35 B = 200 B)
    // This matches HIGHWAY.cc default: {200-35, 200-35, ...}
    PeriodicPKTs_Size = {200 - 35, 200 - 35, 200 - 35, 200 - 35, 200 - 35};
    LargestPeriodicSize = PeriodicPKTs_Size[0];

    Ptr<UniformRandomVariable> random_index = CreateObject<UniformRandomVariable> ();
    for (uint32_t i = 0; i < ueCount; i++)
      Pattern_index.push_back (random_index->GetInteger (0, PeriodicPKTs_Size.size () - 1));
  }

  MeasInterval = ((double)TrepPrint) / 1000.0;

  // ------ install LTE devices --------------------------------------
  NetDeviceContainer ueDevs = lteHelper->InstallUeDevice (ueResponders);

  for (NodeContainer::Iterator L = ueResponders.Begin (); L != ueResponders.End (); ++L)
    {
      Ptr<Node> node = *L;
      Ptr<NetDevice> dev = node->GetDevice (0);
      Ptr<NistLteUeNetDevice> netDev = dev->GetObject<NistLteUeNetDevice> ();
      Ptr<NrV2XUeMac> mac = netDev->GetMac ();
      mac->PushNewRRIValue (RRI);
    }

  // ------ IP stack -------------------------------------------------
  InternetStackHelper internet;
  internet.Install (ueResponders);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.0.0", "255.255.0.0");
  Ipv4InterfaceContainer intcont = ipv4.Assign (ueDevs);

  for (uint32_t u = 0; u < ueResponders.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueResponders.Get (u);
      Ptr<Ipv4StaticRouting> ueStaticRouting =
          ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  lteHelper->Attach (ueDevs);
  BuildingsHelper::Install (ueResponders);
  BuildingsHelper::MakeMobilityModelConsistent ();

  // ------ sidelink bearer configuration ---------------------------
  groupAddress = "225.0.0.1";
  uint32_t groupL2Address = 0x00;

  Ptr<NistSlTft> tftRx = Create<NistSlTft> (NistSlTft::BIDIRECTIONAL, groupAddress, groupL2Address);
  proseHelper->ActivateSidelinkBearer (Seconds (0.001), ueDevs, tftRx);

  Ptr<LteUeRrcSl> ueSidelinkConfiguration = CreateObject<LteUeRrcSl> ();
  ueSidelinkConfiguration->SetSlEnabled (true);

  NistLteRrcSap::SlPreconfiguration preconfiguration;
  preconfiguration.preconfigGeneral.carrierFreq = 54900;
  preconfiguration.preconfigGeneral.slBandwidth = channelBW_RBs;
  preconfiguration.preconfigComm.nbPools = 1;

  NistSlPreconfigPoolFactory pfactory;
  uint64_t pscchBitmapValue = 0x0;
  for (uint32_t i = 0; i < (uint32_t)pscchLength; i++)
    pscchBitmapValue = pscchBitmapValue >> 1 | 0x8000000000;

  pfactory.SetControlBitmap (pscchBitmapValue);
  pfactory.SetControlPeriod (period);
  pfactory.SetDataOffset (pscchLength);
  pfactory.SetHaveUeSelectedResourceConfig (false);
  preconfiguration.preconfigComm.pools[0] = pfactory.CreatePool ();
  ueSidelinkConfiguration->SetSlPreconfiguration (preconfiguration);
  lteHelper->InstallSidelinkConfiguration (ueDevs, ueSidelinkConfiguration);

  // ------ install UDP applications ---------------------------------
  UdpClientHelper udpClient (groupAddress, 8000);
  udpClient.SetAttribute ("MaxPackets", UintegerValue (100000));
  // PATCH 2026-05-28: Interval = CAM generation period (was RRI).
  udpClient.SetAttribute ("Interval", TimeValue (MilliSeconds (CAMInterval)));
  udpClient.SetAttribute ("PacketSize", UintegerValue (200)); // fake, overridden in Send()

  ApplicationContainer clientApps = udpClient.Install (ueResponders);
  Ptr<UniformRandomVariable> rand = CreateObject<UniformRandomVariable> ();
  for (uint32_t i = 0; i < ueResponders.GetN (); i++)
    {
      clientApps.Get (i)->SetStartTime (Seconds (rand->GetValue (0.001, 1.0)));
      clientApps.Get (i)->SetStopTime (Seconds (simTime + 0.5));
    }

  PacketSinkHelper clientPacketSinkHelper ("ns3::UdpSocketFactory",
                                           InetSocketAddress (Ipv4Address::GetAny (), 8000));
  ApplicationContainer clientRespondersSrvApps = clientPacketSinkHelper.Install (ueResponders);
  clientRespondersSrvApps.Start (Seconds (0.001));
  clientRespondersSrvApps.Stop (Seconds (simTime + 0.9));

  // ------ schedule periodic position logging -----------------------
  Simulator::Schedule (MilliSeconds (TrepPrint), &Print, ueResponders);

  // ------ run simulation -------------------------------------------
  NS_LOG_UNCOND ("Starting simulation for " << simTime << " s ...");
  __ph_mark("pre_run");
  Simulator::Stop (Seconds (simTime + 1));
  Simulator::Run ();
  __ph_mark("run_done");
  Simulator::Destroy ();
  __ph_mark("destroyed");

  NS_LOG_INFO ("Done. Results in: " << outputPath);
  return 0;
}
