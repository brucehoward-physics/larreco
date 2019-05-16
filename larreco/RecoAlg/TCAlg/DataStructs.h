////////////////////////////////////////////////////////////////////////
//
//
// TCAlg data structs
//
// Bruce Baller
//
///////////////////////////////////////////////////////////////////////
#ifndef TRAJCLUSTERALGDATASTRUCT_H
#define TRAJCLUSTERALGDATASTRUCT_H


// C/C++ standard libraries
#include <array>
#include <vector>
#include <bitset>

// LArSoft libraries
#include "larcore/Geometry/Geometry.h"
#include "lardata/DetectorInfoServices/LArPropertiesService.h"
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "lardataobj/RecoBase/Hit.h"
#include "lardataobj/RecoBase/Wire.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Shower.h"
#include "lardataobj/RecoBase/SpacePoint.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"
#include "nusimdata/SimulationBase/MCParticle.h"

#include "TMVA/Reader.h"

namespace tca {

  using Point3_t = std::array<double, 3>;
  using Vector3_t = std::array<double, 3>;
  using Point2_t = std::array<float, 2>;
  using Vector2_t = std::array<double, 2>;

  // some functions to handle the CTP_t type
  typedef unsigned int CTP_t;
  constexpr unsigned int Tpad = 10; // alignment for CTP sub-items - TPC
  constexpr unsigned int Cpad = 10000; // alignment for CTP sub-items - Cryostat

  inline CTP_t EncodeCTP(unsigned int cryo, unsigned int tpc, unsigned int plane) { return cryo * Cpad + tpc * Tpad + plane; }
  inline CTP_t EncodeCTP(const geo::PlaneID& planeID) { return EncodeCTP(planeID.Cryostat, planeID.TPC, planeID.Plane); }
  inline CTP_t EncodeCTP(const geo::WireID& wireID) { return EncodeCTP(wireID.Cryostat, wireID.TPC, wireID.Plane); }
  geo::PlaneID DecodeCTP(CTP_t CTP);

  /// @{
  /// @name Data structures for the reconstruction results

  /// struct of temporary 2D vertices (end points)
  struct VtxStore {
    Point2_t Pos {{0,0}};
    Point2_t PosErr {{2,1}};
    unsigned short NTraj {0};
    unsigned short Pass {0};   // Pass in which this vertex was created
    float ChiDOF {0};
    // Topo: 0 = end0-end0, 1 = end0(1)-end1(0), 2 = end1-end1, 3 = CI3DV,
    //       4 = C3DIVIG, 5 = FHV, 6 = FHV2, 7 = SHCH, 8 = CTBC, 9 = Junk, 10 = 3D split, 11 = neutral decay (pizero)
    //       12 = BraggSplit
    short Topo {0};
    CTP_t CTP {0};
    int ID {0};          ///< set to 0 if killed
    int UID {0};          ///< unique global ID
    int Vx3ID {0};
    float Score {0};
    float TjChgFrac {0};            ///< Fraction of charge near the vertex that is from hits on the vertex Tjs
    std::bitset<16> Stat {0};        ///< Vertex status bits using kVtxBit_t
  };

  typedef enum {
    kVtxTrjTried,     ///< FindVtxTraj algorithm tried
    kFixed,           ///< vertex position fixed manually - no fitting done
    kOnDeadWire,
    kHiVx3Score,      ///< matched to a high-score 3D vertex
    kVtxTruMatch,      ///< tagged as a vertex between Tjs that are matched to MC truth neutrino interaction particles
    kVtxMerged,
    kVtxIndPlnNoChg,  ///< vertex quality is suspect - No requirement made on chg btw it and the Tj
    kVtxBitSize     ///< don't mess with this line
  } VtxBit_t;

  /// struct of temporary 3D vertices
  struct Vtx3Store {
    float X {0};                    // x position
    float XErr {0.5};                 // x position error
    float Y {0};                    // y position
    float YErr {0.5};                 // y position error
    float Z {0};                    // z position
    float ZErr {0.5};                 // z position error
    float Score {0};
    short Wire {-1};                 // wire number for an incomplete 3D vertex
    geo::TPCID TPCID;
    std::vector<int> Vx2ID; // List of 2D vertex IDs in each plane
    int ID {0};          // 0 = obsolete vertex
    int UID {0};          ///< unique global ID
    bool Primary {false};
    bool Neutrino {false};
  };

  // A temporary struct for matching trajectory points; 1 struct for each TP for
  // each trajectory. These are put into mallTraj which is then sorted by increasing xlo
  struct Tj2Pt{
    Vector2_t dir;
    unsigned int wire;
    // x range spanned by hits on the TP
    float xlo;
    float xhi;
    CTP_t ctp;
    // the Trajectory ID
    unsigned short id;
    unsigned short ipt; // The trajectory point
    // the number of points in the Tj so that the minimum Tj length cut (MatchCuts[2]) can be made
    unsigned short npts;
    short score; // 0 = Tj with nice vertex, 1 = high quality Tj, 2 = normal, -1 = already matched
  };

  struct TrajPoint {
    CTP_t CTP {0};                   ///< Cryostat, TPC, Plane code
    Point2_t HitPos {{0,0}}; // Charge weighted position of hits in wire equivalent units
    Point2_t Pos {{0,0}}; // Trajectory position in wire equivalent units
    Vector2_t Dir {{0,0}}; // Direction cosines in the StepDir direction
    double HitPosErr2 {0};         // Uncertainty^2 of the hit position perpendiclar to the direction
    // HitPosErr2 < 0 = HitPos not defined because no hits used
    double Ang {0};                // Trajectory angle (-pi, +pi)
    double AngErr {0.1};             // Trajectory angle error
    float Chg {0};                // Chargetj2pt
    float AveChg {-1};             // Average charge of last ~20 TPs
    float ChgPull {0.1};          //  = (Chg - AveChg) / ChgRMS
    float Delta {0};              // Deviation between trajectory and hits (WSE)
    float DeltaRMS {0.02};           // RMS of Deviation between trajectory and hits (WSE)
    float FitChi {0};             // Chi/DOF of the fit
    unsigned short NTPsFit {2}; // Number of trajectory points fitted to make this point
    unsigned short Step {0};      // Step number at which this TP was created
    unsigned short AngleCode {0};          // 0 = small angle, 1 = large angle, 2 = very large angle
    std::vector<unsigned int> Hits; // vector of fHits indices
    std::bitset<16> UseHit {0};   // set true if the hit is used in the fit
    std::bitset<8> Environment {0};    // TPEnvironment_t bitset that describes the environment, e.g. nearby showers or other Tjs
  };

  // struct filled by FitChg
  struct ChgFit {
    Point2_t Pos {{0,0}}; // position origin of the fit
    float Chg;
    float ChgErr;
    float ChgSlp;       // slope relative to the origin
    float ChgSlpErr;
    float ChiDOF;
    unsigned short nPtsFit;
  };

  // Global information for the trajectory
  struct Trajectory {
    std::vector<TrajPoint> Pts;    ///< Trajectory points
    CTP_t CTP {0};                      ///< Cryostat, TPC, Plane code
    std::bitset<128> AlgMod;        ///< Bit set if algorithm AlgBit_t modifed the trajectory
    int WorkID {0};
    int ParentID {-1};     ///< ID of the parent, or the ID of the Tj this one was merged with if it is killed
    float AveChg {0};                   ///< Calculated using ALL hits
    float TotChg {0};                   ///< Total including an estimate for dead wires
    float ChgRMS {0.5};                 /// Normalized RMS using ALL hits. Assume it is 50% to start
    float DirFOM {0.5};         ///< confidence level that the Tj points are ordered correctly using  charge pattern
    short MCSMom {0};         //< Crude 2D estimate to use for shower-like vs track-like discrimination
    float EffPur {0};                     ///< Efficiency * Purity
    Point2_t dEdx {{0,0}};      ///< dE/dx for 3D matched trajectories
    std::array<unsigned short, 2> VtxID {{0,0}};      ///< ID of 2D vertex
    std::array<unsigned short, 2> EndPt {{0,0}}; ///< First and last point in the trajectory that has charge
    int ID;                 ///< ID that is local to one slice
    int UID;                ///< a unique ID for all slices
    int SSID {0};          ///< ID of a 2D shower struct that this tj is in
    unsigned short PDGCode {0};            ///< shower-like or track-like {default is track-like}
    unsigned short Pass {0};            ///< the pass on which it was created
    short StepDir {0};                 ///< -1 = going US (-> small wire#), 1 = going DS (-> large wire#)
    short StartEnd {-1};               ///< The starting end (-1 = don't know)
    unsigned int mcpIndex {UINT_MAX};
    std::array<std::bitset<8>, 2> StopFlag {};  // Bitset that encodes the reason for stopping
    std::bitset<8> Strategy {};        ///
    bool NeedsUpdate {false};          ///< Set true when the Tj needs to be updated
    bool IsGood {true};           ///< set false if there is a failure or the Tj fails quality cuts
    bool MaskedLastTP {false};
  };

  struct TjForecast {
    unsigned short nextForecastUpdate {0};  ///< Revise the forecast when NumPtsWithCharge == nextForecastUpdate
    float showerLikeFraction {0};    ///< fraction of points in the forecast envelope that are shower-like
    float outlook {-1};                     ///< tracklike ~< 2, showerlike > 2
    float chgSlope {0};
    float chgSlopeErr {0};
    float chgFitChiDOF {0};
    float chgRMS {0};
    short MCSMom {0};
    bool leavesBeforeEnd {false};    ///< leaves the forecast envelope before the end
    bool foundShower {false};
    bool endBraggPeak {false};
  };

  // struct used for TrajCluster 3D trajectory points
  struct TrajPoint3 {
    Point3_t Pos {{ 0.0, 0.0, 0.0 }};
    Vector3_t Dir  {{ 0.0, 0.0, 0.0 }};
    std::vector<Tj2Pt> Tj2Pts;  // list of trajectory points
    float dEdx {0};             // The charge is stored here before dE/dx is calculated
    Point2_t AlongTrans;         // Longitudinal & transverse position (cm) relative to the trajectory
  };

  // Struct for 3D trajectory matching
  struct MatchStruct {
    // IDs of Trajectories that match in all planes
    std::vector<int> TjIDs;
    std::vector<float> TjCompleteness;  // fraction of TP points that are 3D-matched
    // Count of the number of X-matched hits and de-weight by angle
    float Count {0};                    // Set to 0 if matching failed
    Point3_t Pos;               // Position center using 3D-matched points on the Tjs - 3D fit
    Vector3_t Dir;              // Direction using 3D-matched points on the Tjs - 3D fit
  };

  struct PFPStruct {
    std::vector<int> TjIDs;             // used to reference Tjs within a slice
    std::vector<int> TjUIDs;             // used to reference Tjs in any slice
    std::vector<float> TjCompleteness;  // fraction of TP points that are 3D-matched
    std::vector<TrajPoint3> Tp3s;    // TrajCluster 3D trajectory points
    // Start is 0, End is 1
    std::array<Point3_t, 2> XYZ;        // XYZ position at both ends (cm)
    std::array<Vector3_t, 2> Dir;
    std::array<Vector3_t, 2> DirErr;
    std::array<std::vector<float>, 2> dEdx;
    std::array<std::vector<float>, 2> dEdxErr;
    std::array<int, 2> Vx3ID {{ 0, 0 }};
    int BestPlane {-1};
    // stuff for constructing the PFParticle
    int PDGCode {-1};
    std::vector<int> DtrUIDs;
    size_t ParentUID {0};       // Parent PFP UID (or 0 if no parent exists)
    geo::TPCID TPCID;
    float EffPur {0};                     ///< Efficiency * Purity
    unsigned int mcpIndex {UINT_MAX};
    unsigned short MatchVecIndex {USHRT_MAX};
    float CosmicScore{0};
    int ID {0};
    int UID {0};              // unique global ID
    bool Primary;             // PFParticle is attached to a primary vertex
    bool NeedsUpdate {true};    // Set true if the PFParticle needs to be (re-)defined
  };

  struct ShowerPoint {
    Point2_t Pos;       // Hit Position in the normal coordinate system
    Point2_t RotPos;    // Position rotated into the shower coordinate system (0 = along, 1 = transverse)
    float Chg {0};                      // Charge of this point
    unsigned int HitIndex;                       // the hit index
    unsigned short TID;             // The ID of the tj the point (hit) is in. TODO eliminate this redundant variable
  };

  // A temporary structure that defines a 2D shower-like cluster of trajectories
  struct ShowerStruct {
    CTP_t CTP;
    int ShowerTjID {0};      // ID of the shower Trajectory composed of many InShower Tjs
    std::vector<int> TjIDs;  // list of InShower Tjs
    std::vector<int> NearTjIDs;   // list of Tjs that are not InShower but satisfy the maxSep cut
    std::vector<ShowerPoint> ShPts;    // Trajectory points inside the shower
    float Angle {0};                   // Angle of the shower axis
    float AngleErr {3};                 // Error
    float AspectRatio {1};              // The ratio of charge weighted transverse/longitudinal positions
    float DirectionFOM {1};
    std::vector<Point2_t> Envelope; // Vertices of a polygon that encompasses the shower
    float EnvelopeArea {0};
    float ChgDensity {0};                   // Charge density inside the Envelope
    float Energy {0};
    float ParentFOM {10};
    int ID {0};
    int UID {0};          ///< unique global ID
    int ParentID {0};  // The ID of a parent Tj - the one at the start of the shower
    int TruParentID {0};
    int SS3ID {0};     // ID of a ShowerStruct3D to which this 2D shower is matched
    bool NeedsUpdate {true};       // Needs to be updated (e.g. after adding a tj, defining a parent, etc)
  };

  // Shower variables filled in MakeShowers. These are in cm and radians
  struct ShowerStruct3D {
    Vector3_t Dir;              //
    Vector3_t DirErr;           // DirErr is hijacked to store the shower rms at the start, center and end sections
    Point3_t Start;               //
    Point3_t StartErr;            // PosErr is hijacked to temporarily store the charge in the three sections
    Point3_t ChgPos;            // position of the center of charge
    Point3_t End;            // end position
    double Len {1};
    double OpenAngle {0.12};
    std::vector<double> Energy;
    std::vector<double> EnergyErr;
    std::vector<double> MIPEnergy;
    std::vector<double> MIPEnergyErr;
    std::vector<double> dEdx;
    std::vector<double> dEdxErr;
    geo::TPCID TPCID;
    std::vector<int> CotIDs;  // list of indices of 2D showers in tjs.cots
    std::vector<unsigned int> Hits;
    int BestPlane;
    int ID;
    int UID {0};          ///< unique global ID
    int ParentID {0};       // The ID of a track-like pfp at the start of the shower, e.g. an electron
    float MatchFOM;
    unsigned short PFPIndex {USHRT_MAX};    // The index of the pfp for this shower
    int Vx3ID {0};
    bool NeedsUpdate {true};       // This is set true whenever the shower needs to be updated
    bool Cheat {false};
  };

  struct DontClusterStruct {
    std::array<int, 2> TjIDs;     // pairs of Tjs that shouldn't be clustered in shower reconstruction because...
    int Vx2ID;                    // they share a 2D vertex that may be matched to...
    int Vx3ID;                    // a high-score 3D vertex
  };

  struct ShowerTreeVars {
    // run, subrun, and event are also saved to this tree

    std::vector<float> BeginWir;   // begin wire
    std::vector<float> BeginTim;   // begin tick
    std::vector<float> BeginAng;   // begin angle
    std::vector<float> BeginChg;   // beginning average charge
    std::vector<short> BeginVtx;   // ID of begin vertex
    std::vector<float> EndWir;   // end wire
    std::vector<float> EndTim;   // end tick
    std::vector<float> EndAng;   // end angle
    std::vector<float> EndChg;   // ending average charge
    std::vector<short> EndVtx;   //ID of end vertex

    std::vector<short> MCSMom;

    std::vector<short> PlaneNum;

    std::vector<int> TjID;
    std::vector<int> IsShowerTj; // indicates tj is an shower trajectory
    std::vector<int> ShowerID; // shower ID associated w/ trajectory. -1 = no shower
    std::vector<int> IsShowerParent; // this tj was chosen as a parent tj
    std::vector<int> StageNum; // stage of reconstruction
    std::vector<std::string> StageName; // stage name

    // envelope information
    std::vector<float> Envelope;
    std::vector<int>EnvPlane;
    std::vector<int>EnvStage;
    std::vector<int>EnvShowerID;

    int nStages {0};
    unsigned short nPlanes {0};

  };

  struct CRTreeVars {
    std::vector<int>   cr_origin;
    std::vector<float> cr_pfpxmin;
    std::vector<float> cr_pfpxmax;
    std::vector<float> cr_pfpyzmindis;
  };

  // Algorithm modification bits
  typedef enum {
    kMaskHits,
    kMaskBadTPs,
    kMichel,
    kDeltaRay,
    kCTKink,        ///< kink found in CheckTraj
    kCTStepChk,
    kTryWithNextPass,
    kRvPrp,
    kCHMUH,
    kSplit,
    kComp3DVx,
    kComp3DVxIG,
    kHED, // High End Delta
    kHamVx,
    kHamVx2,
    kJunkVx,
    kJunkTj,
    kKilled,
    kMerge,
    kTEP,
    kCHMEH,
    kFillGap,
    kUseGhostHits,
    kMrgGhost,
    kChkInTraj,
    kStopBadFits,
    kFixBegin,
    kFTBChg,
    kBeginChg,
    kFixEnd,
    kBraggSplit,
    kUUH,
    kVtxTj,
    kChkVxTj,
    kMisdVxTj,
    kPhoton,
    kHaloTj,
    kNoFitToVx,
    kVxMerge,
    kVxNeutral,
    kNoKinkChk,
    kSoftKink,
    kChkStop,
    kChkStopEP,
    kChkChgAsym,
    kFTBRvProp,
    kStopAtTj,
    kMat3D,
    kM3DVxTj,
    kMat3DMerge,
    kSplit3DKink,
    kTjHiVx3Score,
    kVtxHitsSwap,
    kSplitHiChgHits,
    kShowerLike,
    kKillInShowerVx,
    kShowerTj,
    kShwrParent,
    kMergeOverlap,
    kMergeSubShowers,
    kMergeSubShowersTj,
    kMergeNrShowers,
    kMergeShChain,
    kCompleteShower,
    kSplitTjCVx,
    kNewStpCuts,
    kNewVtxCuts,
    kAlgBitSize     ///< don't mess with this line
  } AlgBit_t;

  typedef enum {
    kNormal,
    kStiffEl,       ///< use the stiff electron strategy
    kStiffMu,       ///< use the stiff muon strategy
    kSlowing        ///< use the slowing-down strategy
  } Strategy_t;

  // Stop flag bits
  typedef enum {
    kSignal,
    kAtKink,
    kAtVtx,
    kBragg,
    kAtTj,
    kOutFV,
    kFlagBitSize     ///< don't mess with this line
  } StopFlag_t;

  // Environment near a trajectory point
  typedef enum {
    kEnvDeadWire,
    kEnvNearTj,
    kEnvNearShower,
    kEnvOverlap,
    kEnvUnusedHits,
    kEnvClean,      ///< the charge fraction is small near this point
    kEnvFlag       ///< a general purpose flag bit used in 3D matching
  } TPEnvironment_t;

  // TrajClusterAlg configuration bits
  typedef enum {
    kStepDir,         ///< step from US -> DS (true) or DS -> US (false)
    kTestBeam,        ///< Expect tracks entering from the front face. Don't create neutrino PFParticles
    kDebug,           ///< master switch for turning on debug mode
    kStudy1,           ///< call study functions to develop cuts, etc (see TCTruth.cxx)
    kStudy2,           ///< call study functions to develop cuts, etc
    kStudy3,           ///< call study functions to develop cuts, etc
    kStudy4,           ///< call study functions to develop cuts, etc
    kSaveCRTree,      ///< save cosmic ray tree
    kTagCosmics,      ///< tag cosmic rays
    kSaveShowerTree  ///< save shower tree
  } TCModes_t;

  extern const std::vector<std::string> AlgBitNames;
  extern const std::vector<std::string> StopFlagNames;
  extern const std::vector<std::string> VtxBitNames;
  extern const std::vector<std::string> StrategyBitNames;

  // struct for configuration - used in all slices
  struct TCConfig {
    std::vector<float> vtx2DCuts; ///< Max position pull, max Position error rms
    std::vector<float> vtx3DCuts;   ///< 2D vtx -> 3D vtx matching cuts
    std::vector<float> vtxScoreWeights;
    std::vector<float> neutralVxCuts;
    std::vector<short> deltaRayTag; ///< min length, min MCSMom and min separation (WSE) for a delta ray tag
    std::vector<short> muonTag; ///< min length and min MCSMom for a muon tag
    std::vector<float> electronTag;
    std::vector<float> chkStopCuts; ///< [Min Chg ratio, Chg slope pull cut, Chg fit chi cut]
    std::vector<float> showerTag; ///< [min MCSMom, max separation, min # Tj < separation] for a shower tag
    std::vector<float> kinkCuts; ///< kink angle, nPts fit, (alternate) kink angle significance
    std::vector<float> match3DCuts;  ///< 3D matching cuts
    std::vector<float> matchTruth;     ///< Match to MC truth
    std::vector<float> chargeCuts;
    std::vector<float> qualityCuts; ///< Min points/wire, min consecutive pts after a gap
    std::vector<float> pfpStitchCuts;      ///< cuts for stitching between TPCs
    std::vector<float> testBeamCuts;
    std::vector<unsigned short> minPtsFit; ///< Reconstruct in several passes
    std::vector<unsigned short> minPts;    ///< min number of Pts required to make a trajectory
    std::vector<unsigned short> maxAngleCode;   ///< max allowed angle code for each pass
    std::vector<short> minMCSMom;   ///< Min MCSMom for each pass
    std::vector<float> angleRanges; ///< list of max angles for each angle range
    float wirePitch;
    float unitsPerTick;     ///< scale factor from Tick to WSE equivalent units
    std::vector<float> maxPos0;
    std::vector<float> maxPos1;
    float multHitSep;      ///< preferentially "merge" hits with < this separation
    float maxChi;
    const geo::GeometryCore* geom;
    const detinfo::DetectorProperties* detprop;
    calo::CalorimetryAlg* caloAlg;
    TMVA::Reader* showerParentReader;
    std::vector<float> showerParentVars;
    float hitErrFac;
    float maxWireSkipNoSignal;    ///< max number of wires to skip w/o a signal on them
    float maxWireSkipWithSignal;  ///< max number of wires to skip with a signal on them
    float projectionErrFactor;
    float VLAStepSize;
    float JTMaxHitSep2;  /// Max hit separation for making junk trajectories. < 0 to turn off
    std::bitset<128> useAlg;  ///< Allow user to mask off specific algorithms
    std::bitset<128> dbgAlg;  ///< Allow user to turn on debug printing in algorithms (that print...)
    short recoSlice {0};     ///< only reconstruct the slice with ID (0 = all)
    bool dbgSlc {true};          ///< debug only in the user-defined slice? default is all slices
    bool dbgStp {false};          ///< debug stepping using debug.Cryostat, debug.TPC, etc
    bool dbgMrg {false};
    bool dbg2V {false};           ///< debug 2D vertex finding
    bool dbgVxNeutral {false};
    bool dbgVxMerge {false};
    bool dbgVxJunk {false};
    bool dbg3V {false};           ///< debug 3D vertex finding
    bool dbgPFP {false};
    bool dbgDeltaRayTag {false};
    bool dbgMuonTag {false};
    bool dbg2S {false};
    bool dbg3S {false};
    bool dbgStitch {false};    ///< debug PFParticle stitching
    bool dbgSummary {false};    ///< print a summary report
    bool dbgDump {false};   /// dump trajectory points
    short nPtsAve;         /// number of points to find AveChg
    std::bitset<16> modes;   /// See TCMode_t above
    bool doForecast {false};
  };

  struct TCHit {
    unsigned int allHitsIndex; // index into fHits
    int InTraj {0};     // ID of the trajectory this hit is used in, 0 = none, < 0 = Tj under construction
  };

  // lower/upper range of hits indexed into allHits for a CTP - wire pair
  struct AllHitsRange {
    CTP_t CTP;
    unsigned int wire {UINT_MAX};
    unsigned int firstHit {UINT_MAX};
    unsigned int lastHit {UINT_MAX};
  };

  struct SptHits {
    unsigned int sptIndex {UINT_MAX};                   ///< index into SpacePoint collection offset by sptHandle
    std::array<unsigned int, 3> allHitsIndex {{UINT_MAX}}; ///< index into allHits collection for each plane
  };

  // hit collection for all slices, TPCs and cryostats + event information
  // Note: Ideally this hit collection would be the FULL hit collection before cosmic removal
  struct TCEvent {
    std::vector<recob::Hit> const* allHits = nullptr;
    std::vector<AllHitsRange> allHitsRanges;
    std::vector<simb::MCParticle> const* mcpHandle = nullptr;  ///< handle to MCParticles in the event
    std::vector<recob::SpacePoint> const* sptHandle = nullptr; ///< handle to SpacePoints in the event
    std::vector<SptHits> const* sptHits = nullptr;           ///< pointer to the spacepoint - hit vector
    std::vector<unsigned int> allHitsMCPIndex;               ///< index of matched hits into the MCParticle vector
    unsigned int event;
    unsigned int run;
    unsigned int subRun;
    unsigned int eventsProcessed;
    std::vector<float> aveHitRMS;      ///< average RMS of an isolated hit
    int WorkID;
    int globalTjID;
    int globalPFPID;
    int globalVx2ID;
    int globalVx3ID;
    int globalS2ID;
    int globalS3ID;
    bool aveHitRMSValid {false};          ///< set true when the average hit RMS is well-known
  };

  struct TCSlice {
    std::vector<unsigned int> nWires;
    std::vector<unsigned int> firstWire;    ///< the first wire with a hit
    std::vector<unsigned int> lastWire;      ///< the last wire with a hit
    float xLo; // fiducial volume of the current tpc
    float xHi;
    float yLo;
    float yHi;
    float zLo;
    float zHi;
    geo::TPCID TPCID;
    unsigned short nPlanes;
    int ID;           ///< ID of the recob::Slice (not the sub-slice)
    // The variables below do change in size from event to event

    // Save histograms to develop cosmic removal tools
    CRTreeVars crt;
    std::vector<TCHit> slHits;
    std::vector<Trajectory> tjs; ///< vector of all trajectories in each plane
    std::vector<Tj2Pt> mallTraj;      ///< vector of trajectory points ordered by increasing X
    // vector of pairs of first (.first) and last+1 (.second) hit on each wire
    // in the range fFirstWire to fLastWire. A value of -2 indicates that there
    // are no hits on the wire. A value of -1 indicates that the wire is dead
    std::vector<std::vector< std::pair<int, int>>> wireHitRange;
    std::vector< VtxStore > vtxs; ///< 2D vertices
    std::vector< Vtx3Store > vtx3s; ///< 3D vertices
    std::vector<MatchStruct> matchVec; ///< 3D matching vector
    std::vector<PFPStruct> pfps;
    std::vector<ShowerStruct> cots;       // Clusters of Trajectories that define 2D showers
    std::vector<DontClusterStruct> dontCluster; // pairs of Tjs that shouldn't clustered in one shower
    std::vector<ShowerStruct3D> showers;  // 3D showers
    bool isValid {false};                 // set false if this slice failed reconstruction
   };

  extern TCEvent evt;
  extern TCConfig tcc;
  extern ShowerTreeVars stv;
  extern std::vector<TjForecast> tjfs;

  // vector of hits, tjs, etc in each slice
  extern std::vector<TCSlice> slices;
  // vector of seed TPs
  extern std::vector<TrajPoint> seeds;

} // namespace tca

#endif // ifndef TRAJCLUSTERALGDATASTRUCT_H
