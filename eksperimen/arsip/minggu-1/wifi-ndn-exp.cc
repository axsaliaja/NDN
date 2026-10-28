// NDN pada WiFi ad hoc untuk komunikasi darurat - skenario 5 node (ndnSIM 2.9).
//
//   Posko(producer) ~~ Relawan1 ~~ Relawan2 ~~ PengungsiA ~~ PengungsiB
//   berjajar tiap `spacing` m, jangkauan radio `range` m -> hanya tetangga langsung yang terjangkau.
//
//   --fix=0                         face WiFi default ndnSIM (point-to-point)
//   --fix=1 --strategy=multicast    face WiFi AD_HOC (syarat multi-hop)
//   --fix=1 --strategy=jitter-broadcast --jitter=10   + jeda rebroadcast acak (usulan)
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/apps/ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-net-device-transport.hpp"
#include "ns3/ndnSIM/NFD/daemon/face/generic-link-service.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/strategy.hpp"
#include "ns3/ndnSIM/NFD/daemon/fw/algorithm.hpp"

#include <map>
#include <set>

// Strategi usulan: Interest dari aplikasi lokal langsung dikirim, Interest yang di-relay ditunda
// acak U(0, maxJitter) ms. Kalau selama menunggu Data sudah lewat (PIT terpenuhi), rebroadcast
// dibatalkan -> memecah tabrakan sinkron (hidden terminal) & mengurangi transmisi berlebih.
namespace nfd {
namespace fw {
class JitterBroadcastStrategy : public Strategy {
public:
  static double s_maxJitterMs;

  explicit JitterBroadcastStrategy(Forwarder& f, const Name& name = getStrategyName())
    : Strategy(f)
  {
    this->setInstanceName(makeInstanceName(name, getStrategyName()));
  }
  static const Name& getStrategyName()
  {
    static Name n("/localhost/nfd/strategy/jitter-broadcast/%FD%01");
    return n;
  }
  void afterReceiveInterest(const Interest& interest, const FaceEndpoint& ingress,
                            const shared_ptr<pit::Entry>& pitEntry) override
  {
    if (hasPendingOutRecords(*pitEntry))
      return;
    bool local = ingress.face.getScope() == ndn::nfd::FACE_SCOPE_LOCAL;
    for (const auto& nh : this->lookupFib(*pitEntry).getNextHops()) {
      if (!isNextHopEligible(ingress.face, interest, nh, pitEntry))
        continue;
      Face& out = nh.getFace();
      if (local || s_maxJitterMs <= 0) {
        this->sendInterest(interest, out, pitEntry);
        continue;
      }
      auto rv = ns3::CreateObject<ns3::UniformRandomVariable>();
      std::weak_ptr<pit::Entry> weak = pitEntry;
      ns3::Simulator::Schedule(ns3::MicroSeconds(rv->GetValue(0, s_maxJitterMs * 1000)),
                               [this, weak, &out] {
        auto pe = weak.lock();
        if (!pe || pe->isSatisfied || pe->getInRecords().empty() || hasPendingOutRecords(*pe))
          return;
        // Interest yang disimpan PIT dimiliki shared_ptr (dibutuhkan shared_from_this di face)
        this->sendInterest(pe->getInterest(), out, pe);
      });
    }
  }
};
double JitterBroadcastStrategy::s_maxJitterMs = 10;
NFD_REGISTER_STRATEGY(JitterBroadcastStrategy);
} // namespace fw
} // namespace nfd

namespace ns3 {

// Face WiFi dengan link type AD_HOC: NFD boleh meneruskan Interest/Data keluar lewat face yang
// sama dengan face masuknya (syarat multi-hop di WiFi ad hoc). Default ndnSIM = point-to-point.
static std::shared_ptr<ndn::Face>
AdhocWifiFace(Ptr<Node> node, Ptr<ndn::L3Protocol> l3, Ptr<NetDevice> dev)
{
  ::nfd::face::GenericLinkService::Options opts;
  opts.allowFragmentation = true;
  opts.allowReassembly = true;
  auto ls = std::make_unique<::nfd::face::GenericLinkService>(opts);
  std::ostringstream uri;
  uri << "netdev://[" << Mac48Address::ConvertFrom(dev->GetAddress()) << "]";
  auto tr = std::make_unique<ndn::NetDeviceTransport>(node, dev, uri.str(),
                                                      "netdev://[ff:ff:ff:ff:ff:ff]",
                                                      ::ndn::nfd::FACE_SCOPE_NON_LOCAL,
                                                      ::ndn::nfd::FACE_PERSISTENCY_PERSISTENT,
                                                      ::ndn::nfd::LINK_TYPE_AD_HOC);
  auto face = std::make_shared<ndn::Face>(std::move(ls), std::move(tr));
  face->setMetric(1);
  l3->addFace(face);
  return face;
}

// ---- statistik per consumer ----
struct Stat {
  std::set<uint32_t> sent, got;
  double delaySum = 0;
};
static std::map<std::string, Stat> g_stat;
static double g_measureEnd = 0;
static uint64_t g_macTx = 0;

static void OnTx(std::string who, std::shared_ptr<const ndn::Interest> i, Ptr<ndn::App>,
                 std::shared_ptr<ndn::Face>)
{
  if (Simulator::Now().GetSeconds() < g_measureEnd)
    g_stat[who].sent.insert(i->getName().get(-1).toSequenceNumber());
}
static void OnFirstData(std::string who, Ptr<ndn::App>, uint32_t seq, Time d, uint32_t, int32_t)
{
  auto& s = g_stat[who];
  if (s.sent.count(seq) && s.got.insert(seq).second)
    s.delaySum += d.GetMilliSeconds();
}
static void OnMacTx(Ptr<const Packet>) { ++g_macTx; }

static void Consumer(Ptr<Node> n, std::string prefix, double freq, std::string who)
{
  ndn::AppHelper h("ns3::ndn::ConsumerCbr");
  h.SetPrefix(prefix);
  h.SetAttribute("Frequency", DoubleValue(freq));
  h.SetAttribute("LifeTime", StringValue("2s"));
  h.SetAttribute("Randomize", StringValue("uniform")); // tanpa ini semua node kirim bersamaan
  auto app = DynamicCast<ndn::App>(h.Install(n).Get(0));
  app->SetStartTime(Seconds(1));
  NS_ABORT_UNLESS(app->TraceConnectWithoutContext("TransmittedInterests", MakeBoundCallback(&OnTx, who)));
  NS_ABORT_UNLESS(app->TraceConnectWithoutContext("FirstInterestDataDelay", MakeBoundCallback(&OnFirstData, who)));
  g_stat[who];
}

int Main(int argc, char* argv[])
{
  std::string strategy = "multicast";
  bool fix = true;
  double dur = 20, spacing = 80, range = 100, userFreq = 10;
  uint32_t run = 1;

  CommandLine cmd;
  cmd.AddValue("fix", "pakai face AD_HOC (1) atau default ndnSIM (0)", fix);
  cmd.AddValue("strategy", "multicast | best-route | self-learning | jitter-broadcast", strategy);
  cmd.AddValue("jitter", "jitter-broadcast: jeda maksimum rebroadcast (ms)",
               ::nfd::fw::JitterBroadcastStrategy::s_maxJitterMs);
  cmd.AddValue("dur", "durasi pengukuran (s)", dur);
  cmd.AddValue("spacing", "jarak antar node (m)", spacing);
  cmd.AddValue("range", "jangkauan radio (m)", range);
  cmd.AddValue("userFreq", "laju Interest tiap consumer (Interest/s)", userFreq);
  cmd.AddValue("run", "seed run", run);
  cmd.Parse(argc, argv);
  RngSeedManager::SetRun(run);
  g_measureEnd = dur;

  // 802.11a; frame broadcast (semua paket NDN di ndnSIM) dikirim di basic rate 6 Mbps tanpa ACK
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue("OfdmRate6Mbps"));
  WifiHelper wifi;
  wifi.SetStandard(WIFI_STANDARD_80211a);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue("OfdmRate24Mbps"));
  YansWifiChannelHelper ch;
  ch.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  ch.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(range));
  YansWifiPhyHelper phy;
  phy.SetChannel(ch.Create());
  WifiMacHelper mac;
  mac.SetType("ns3::AdhocWifiMac");

  // 0=Posko(producer) 1=Relawan1 2=Relawan2 3=PengungsiA 4=PengungsiB
  NodeContainer nodes;
  nodes.Create(5);
  wifi.Install(phy, mac, nodes);
  MobilityHelper mob;
  auto pos = CreateObject<ListPositionAllocator>();
  for (int i = 0; i < 5; ++i)
    pos->Add(Vector(i * spacing, 0, 0));
  mob.SetPositionAllocator(pos);
  mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mob.Install(nodes);

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.setCsSize(1000);
  if (fix)
    ndnHelper.AddFaceCreateCallback(WifiNetDevice::GetTypeId(), MakeCallback(&AdhocWifiFace));
  ndnHelper.Install(nodes);
  ndn::StrategyChoiceHelper::Install(nodes, "/", "/localhost/nfd/strategy/" + strategy);

  ndn::AppHelper p("ns3::ndn::Producer");
  p.SetPrefix("/posko");
  p.SetAttribute("PayloadSize", StringValue("1024"));
  p.Install(nodes.Get(0));
  Consumer(nodes.Get(1), "/posko/info/relawan1", userFreq, "Relawan1(1hop)");
  Consumer(nodes.Get(3), "/posko/info/pengungsiA", userFreq, "PengungsiA(3hop)");
  Consumer(nodes.Get(4), "/posko/info/pengungsiB", userFreq, "PengungsiB(4hop)");

  Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTx", MakeCallback(&OnMacTx));

  Simulator::Stop(Seconds(dur + 3));
  Simulator::Run();

  std::cout << "RESULT fix=" << fix << " strategy=" << strategy;
  if (strategy == "jitter-broadcast")
    std::cout << " jitter=" << ::nfd::fw::JitterBroadcastStrategy::s_maxJitterMs;
  std::cout << " run=" << run << "\n";
  for (auto& kv : g_stat) {
    auto& s = kv.second;
    double sat = s.sent.empty() ? 0 : 100.0 * s.got.size() / s.sent.size();
    double d = s.got.empty() ? 0 : s.delaySum / s.got.size();
    std::cout << "  " << kv.first << " sent=" << s.sent.size() << " ok=" << s.got.size()
              << " sat=" << sat << "% delay=" << d << "ms\n";
  }
  std::cout << "  SUMMARY wifiMacTx=" << g_macTx << "\n";
  Simulator::Destroy();
  return 0;
}

} // namespace ns3

int main(int argc, char* argv[])
{
  return ns3::Main(argc, argv);
}
