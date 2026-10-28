// NDN pada WiFi ad hoc untuk komunikasi darurat (ndnSIM 2.9 / NFD 22.02).
// Satu program berparameter untuk semua eksperimen (E1–E7, lihat eksperimen/HASIL.md).
//
//   --topo=chain   n node berjajar tiap `spacing` m (bawaan: 5 node, 80 m)
//   --topo=grid    k×k node (n = k*k), jarak `spacing` m, producer di pojok
//   --topo=random  n node acak di area `area`×`area` m, producer di tengah, diulang sampai terhubung
//   --fix=0        face WiFi bawaan ndnSIM (point-to-point)  |  --fix=1 face AD_HOC
//   --strategy=multicast|best-route|self-learning|jitter-broadcast  (--jitter, --retxSup)
//   --consumers="1,3,4" (indeks node; -1 = node terakhir) atau "rand:K" (K node acak)
//   --shared=1     semua consumer meminta nama yang sama (uji cache/agregasi PIT)
//   --speed=v      (topo random) node selain producer bergerak acak v m/s
//
// Keluaran: baris RESULT (parameter), CONS (per consumer), SUM (lapisan WiFi). Dibaca analisis.py.
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

#include <algorithm>
#include <map>
#include <queue>
#include <set>
#include <sstream>

// Strategi usulan (jitter-broadcast). Interest dari aplikasi lokal langsung dikirim; Interest yang
// diteruskan untuk node lain ditunda acak U(0, maxJitter) ms (RFC 5148). Bila selama menunggu
// Interest itu sudah diteruskan (atau Data sudah lewat), pengiriman dibatalkan.
// retxSup < 0 (bawaan, perilaku minggu 1): Interest ulang dari consumer (Nonce baru) tidak
// diteruskan selama Interest sebelumnya masih tercatat menunggu di PIT (sampai 2 s).
// retxSup >= 0: Interest ulang diteruskan lagi bila Interest terakhir keluar >= retxSup ms lalu.
namespace nfd {
namespace fw {
class JitterBroadcastStrategy : public Strategy {
public:
  static double s_maxJitterMs;
  static double s_retxSupMs;

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
    if (m_waiting.count(interest.getName()))
      return; // sudah dijadwalkan dikirim
    if (hasPendingOutRecords(*pitEntry) && !retxAllowed(*pitEntry))
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
      // Salinan dimiliki shared_ptr (face membutuhkan shared_from_this); Nonce ikut Interest ini
      auto copy = std::make_shared<Interest>(interest);
      auto scheduledAt = time::steady_clock::now();
      std::weak_ptr<pit::Entry> weak = pitEntry;
      m_waiting.insert(interest.getName());
      auto rv = ns3::CreateObject<ns3::UniformRandomVariable>();
      ns3::Simulator::Schedule(ns3::MicroSeconds(rv->GetValue(0, s_maxJitterMs * 1000)),
                               [this, weak, &out, copy, scheduledAt] {
        m_waiting.erase(copy->getName());
        auto pe = weak.lock();
        if (!pe || pe->isSatisfied || pe->getInRecords().empty())
          return;
        if (hasPendingOutRecords(*pe) && getLastOutgoing(*pe) >= scheduledAt)
          return; // sudah diteruskan selama menunggu
        this->sendInterest(*copy, out, pe);
      });
    }
  }

private:
  bool retxAllowed(const pit::Entry& pe) const
  {
    if (s_retxSupMs < 0)
      return false;
    auto sup = time::microseconds(static_cast<int64_t>(s_retxSupMs * 1000));
    return time::steady_clock::now() - getLastOutgoing(pe) >= sup;
  }

  std::set<Name> m_waiting;
};
double JitterBroadcastStrategy::s_maxJitterMs = 10;
double JitterBroadcastStrategy::s_retxSupMs = -1;
NFD_REGISTER_STRATEGY(JitterBroadcastStrategy);
} // namespace fw
} // namespace nfd

namespace ns3 {

// Face WiFi dengan link type AD_HOC: NFD boleh meneruskan paket keluar lewat face tempat paket itu
// masuk (syarat multi-hop di WiFi ad hoc). Bawaan ndnSIM = point-to-point.
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

// ---- statistik ----
struct Stat {
  int hop = 0;
  std::set<uint32_t> sent;
  std::map<uint32_t, std::pair<double, uint32_t>> got; // seq -> (delay ms, jumlah kiriman Interest)
};
static std::map<uint32_t, Stat> g_stat; // per node consumer
static double g_measureEnd = 0;
static uint64_t g_phyTx = 0, g_rxOk = 0, g_rxErr = 0, g_phyDrop = 0;
static std::map<uint32_t, uint64_t> g_rxOkNode, g_rxErrNode, g_dropNode;
static std::map<std::string, uint64_t> g_dropReasonN1;

static uint32_t NodeOf(const std::string& ctx) // "/NodeList/3/DeviceList/..." -> 3
{
  return std::stoul(ctx.substr(10, ctx.find('/', 10) - 10));
}
static void OnTx(uint32_t id, std::shared_ptr<const ndn::Interest> i, Ptr<ndn::App>,
                 std::shared_ptr<ndn::Face>)
{
  if (Simulator::Now().GetSeconds() < g_measureEnd)
    g_stat[id].sent.insert(i->getName().get(-1).toSequenceNumber());
}
static void OnFirstData(uint32_t id, Ptr<ndn::App>, uint32_t seq, Time d, uint32_t retx, int32_t)
{
  auto& s = g_stat[id];
  if (s.sent.count(seq) && !s.got.count(seq))
    s.got[seq] = {d.GetSeconds() * 1000, retx};
}
static void OnPhyTx(std::string, Ptr<const Packet>, double) { ++g_phyTx; }
static void OnRxOk(std::string c, Ptr<const Packet>, double, WifiMode, WifiPreamble)
{
  ++g_rxOk;
  ++g_rxOkNode[NodeOf(c)];
}
static void OnRxErr(std::string c, Ptr<const Packet>, double)
{
  ++g_rxErr;
  ++g_rxErrNode[NodeOf(c)];
}
static void OnPhyDrop(std::string c, Ptr<const Packet>, WifiPhyRxfailureReason r)
{
  ++g_phyDrop;
  uint32_t id = NodeOf(c);
  ++g_dropNode[id];
  if (id == 1) {
    std::ostringstream os;
    os << r;
    ++g_dropReasonN1[os.str()];
  }
}

// Jarak hop dari producer (node 0) pada graf jangkauan awal
static std::vector<int> Hops(const std::vector<Vector>& p, double range)
{
  std::vector<int> h(p.size(), -1);
  std::queue<size_t> q;
  h[0] = 0;
  q.push(0);
  while (!q.empty()) {
    size_t u = q.front();
    q.pop();
    for (size_t v = 0; v < p.size(); ++v)
      if (h[v] < 0 && CalculateDistance(p[u], p[v]) <= range) {
        h[v] = h[u] + 1;
        q.push(v);
      }
  }
  return h;
}

static double Pct(std::vector<double> v, double q)
{
  if (v.empty())
    return 0;
  std::sort(v.begin(), v.end());
  return v[std::min(v.size() - 1, static_cast<size_t>(q * v.size()))];
}

int Main(int argc, char* argv[])
{
  std::string topo = "chain", strategy = "multicast", consumers = "1,3,4";
  bool fix = true, shared = false;
  uint32_t n = 5, run = 1;
  double dur = 20, spacing = 80, range = 100, area = 300, speed = 0, userFreq = 10;

  CommandLine cmd;
  cmd.AddValue("topo", "chain | grid | random", topo);
  cmd.AddValue("n", "jumlah node (grid: dibulatkan ke k*k)", n);
  cmd.AddValue("spacing", "jarak antar node chain/grid (m)", spacing);
  cmd.AddValue("area", "sisi area topologi random (m)", area);
  cmd.AddValue("range", "jangkauan radio (m)", range);
  cmd.AddValue("speed", "kecepatan gerak acak node non-producer, topo random (m/s)", speed);
  cmd.AddValue("fix", "face AD_HOC (1) atau bawaan ndnSIM (0)", fix);
  cmd.AddValue("strategy", "multicast | best-route | self-learning | jitter-broadcast", strategy);
  cmd.AddValue("jitter", "jitter-broadcast: jeda maksimum (ms)",
               ::nfd::fw::JitterBroadcastStrategy::s_maxJitterMs);
  cmd.AddValue("retxSup", "jitter-broadcast: jeda minimum meneruskan Interest ulang (ms); <0 = tidak",
               ::nfd::fw::JitterBroadcastStrategy::s_retxSupMs);
  cmd.AddValue("consumers", "indeks node consumer dipisah koma (-1 = terakhir) atau rand:K", consumers);
  cmd.AddValue("shared", "semua consumer meminta nama yang sama", shared);
  cmd.AddValue("userFreq", "laju Interest tiap consumer (Interest/s)", userFreq);
  cmd.AddValue("dur", "durasi pengukuran (s)", dur);
  cmd.AddValue("run", "seed run", run);
  std::string trace;
  cmd.AddValue("trace", "berkas jejak paket NDN per node/face per detik (L3RateTracer)", trace);
  cmd.Parse(argc, argv);
  RngSeedManager::SetRun(run);
  g_measureEnd = dur;

  // ---- posisi node ----
  std::vector<Vector> pos;
  if (topo == "chain") {
    for (uint32_t i = 0; i < n; ++i)
      pos.push_back(Vector(i * spacing, 0, 0));
  }
  else if (topo == "grid") {
    uint32_t k = std::max<uint32_t>(2, std::lround(std::sqrt(n)));
    n = k * k;
    for (uint32_t i = 0; i < n; ++i)
      pos.push_back(Vector((i % k) * spacing, (i / k) * spacing, 0));
  }
  else if (topo == "random") {
    auto u = CreateObject<UniformRandomVariable>();
    for (int attempt = 0;; ++attempt) {
      NS_ABORT_MSG_IF(attempt > 1000, "topologi acak tidak pernah terhubung");
      pos = {Vector(area / 2, area / 2, 0)};
      for (uint32_t i = 1; i < n; ++i)
        pos.push_back(Vector(u->GetValue(0, area), u->GetValue(0, area), 0));
      auto h = Hops(pos, range);
      if (std::none_of(h.begin(), h.end(), [](int x) { return x < 0; }))
        break;
    }
  }
  else
    NS_ABORT_MSG("topo tidak dikenal: " << topo);
  auto hops = Hops(pos, range);

  // ---- pilih consumer ----
  std::vector<uint32_t> cons;
  if (consumers.rfind("rand:", 0) == 0) {
    uint32_t k = std::stoul(consumers.substr(5));
    std::vector<uint32_t> cand;
    for (uint32_t i = 1; i < n; ++i)
      cand.push_back(i);
    auto u = CreateObject<UniformRandomVariable>();
    for (uint32_t i = 0; i < k && !cand.empty(); ++i) {
      auto j = u->GetInteger(0, cand.size() - 1);
      cons.push_back(cand[j]);
      cand.erase(cand.begin() + j);
    }
  }
  else {
    std::stringstream ss(consumers);
    for (std::string t; std::getline(ss, t, ',');) {
      int v = std::stoi(t);
      cons.push_back(v < 0 ? n + v : v);
    }
  }

  // ---- WiFi: 802.11a ad hoc; frame broadcast dikirim di basic rate 6 Mbps tanpa ACK ----
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

  NodeContainer nodes;
  nodes.Create(n);
  wifi.Install(phy, mac, nodes);

  auto alloc = CreateObject<ListPositionAllocator>();
  for (auto& p : pos)
    alloc->Add(p);
  MobilityHelper mob;
  mob.SetPositionAllocator(alloc);
  mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mob.Install(nodes.Get(0));
  if (speed > 0) {
    NS_ABORT_MSG_IF(topo != "random", "speed hanya untuk topo=random");
    std::ostringstream sp;
    sp << "ns3::ConstantRandomVariable[Constant=" << speed << "]";
    mob.SetMobilityModel("ns3::RandomWalk2dMobilityModel", "Bounds",
                         RectangleValue(Rectangle(0, area, 0, area)), "Speed", StringValue(sp.str()),
                         "Mode", StringValue("Time"), "Time", StringValue("2s"));
  }
  for (uint32_t i = 1; i < n; ++i)
    mob.Install(nodes.Get(i));

  // ---- NDN ----
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
  for (auto id : cons) {
    ndn::AppHelper h("ns3::ndn::ConsumerCbr");
    h.SetPrefix(shared ? "/posko/info/bersama" : "/posko/info/n" + std::to_string(id));
    h.SetAttribute("Frequency", DoubleValue(userFreq));
    h.SetAttribute("LifeTime", StringValue("2s"));
    h.SetAttribute("Randomize", StringValue("uniform")); // tanpa ini semua consumer kirim serempak
    auto app = DynamicCast<ndn::App>(h.Install(nodes.Get(id)).Get(0));
    app->SetStartTime(Seconds(1));
    NS_ABORT_UNLESS(app->TraceConnectWithoutContext("TransmittedInterests", MakeBoundCallback(&OnTx, id)));
    NS_ABORT_UNLESS(app->TraceConnectWithoutContext("FirstInterestDataDelay", MakeBoundCallback(&OnFirstData, id)));
    g_stat[id].hop = hops[id];
  }

  const std::string dev = "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/";
  Config::Connect(dev + "PhyTxBegin", MakeCallback(&OnPhyTx));
  Config::Connect(dev + "State/RxOk", MakeCallback(&OnRxOk));
  Config::Connect(dev + "State/RxError", MakeCallback(&OnRxErr));
  Config::Connect(dev + "PhyRxDrop", MakeCallback(&OnPhyDrop));

  if (!trace.empty())
    ndn::L3RateTracer::InstallAll(trace, Seconds(1));
  Simulator::Stop(Seconds(dur + 3));
  Simulator::Run();

  std::cout << "RESULT topo=" << topo << " n=" << n << " spacing=" << spacing << " area=" << area
            << " range=" << range << " speed=" << speed << " fix=" << fix << " strategy=" << strategy
            << " jitter=" << (strategy == "jitter-broadcast" ? ::nfd::fw::JitterBroadcastStrategy::s_maxJitterMs : -1)
            << " retxSup=" << (strategy == "jitter-broadcast" ? ::nfd::fw::JitterBroadcastStrategy::s_retxSupMs : -1)
            << " shared=" << shared << " userFreq=" << userFreq << " dur=" << dur << " run=" << run << "\n";
  for (auto& kv : g_stat) {
    auto& s = kv.second;
    std::vector<double> d, dFirst, dRetx;
    for (auto& g : s.got) {
      d.push_back(g.second.first);
      // hitungan kiriman ndnSIM mulai dari 1 (= kiriman pertama, tanpa kirim ulang)
      (g.second.second <= 1 ? dFirst : dRetx).push_back(g.second.first);
    }
    auto mean = [](const std::vector<double>& v) {
      double t = 0;
      for (double x : v)
        t += x;
      return v.empty() ? 0 : t / v.size();
    };
    std::cout << "CONS node=" << kv.first << " hop=" << s.hop << " sent=" << s.sent.size()
              << " ok=" << s.got.size() << " sat=" << (s.sent.empty() ? 0 : 100.0 * s.got.size() / s.sent.size())
              << " dmean=" << mean(d) << " dmed=" << Pct(d, 0.5) << " d95=" << Pct(d, 0.95)
              << " firsttry=" << dFirst.size() << " dfirst=" << mean(dFirst) << " dretx=" << mean(dRetx) << "\n";
  }
  std::cout << "SUM phyTx=" << g_phyTx << " rxOk=" << g_rxOk << " rxErr=" << g_rxErr
            << " phyDrop=" << g_phyDrop << " rxOkN1=" << g_rxOkNode[1] << " rxErrN1=" << g_rxErrNode[1] << " dropN1=" << g_dropNode[1] << "\n";
  std::cout << "DROPN1";
  for (auto& kv : g_dropReasonN1)
    std::cout << " " << kv.first << "=" << kv.second;
  std::cout << "\n";
  Simulator::Destroy();
  return 0;
}

} // namespace ns3

int main(int argc, char* argv[])
{
  return ns3::Main(argc, argv);
}
