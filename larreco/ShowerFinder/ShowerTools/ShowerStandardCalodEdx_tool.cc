//############################################################################
//### Name:        ShowerStandardCalodEdx                                  ###
//### Author:      Ed Tyley                                                ###
//### Date:        13.05.19                                                ###
//### Description: Tool for finding the dEdx of the start track of the     ###
//###              shower using the standard calomitry module. Derived     ###
//###              from the EMShower_module.cc                             ###
//############################################################################

#include "larreco/ShowerFinder/ShowerTools/IShowerTool.h"

//Framework Includes
#include "art/Utilities/ToolMacros.h"
#include "art/Utilities/make_tool.h"
#include "art_root_io/TFileService.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib_except/exception.h"
#include "canvas/Persistency/Common/Ptr.h"

//LArSoft Includes 
#include "lardata/DetectorInfoServices/DetectorPropertiesService.h"
#include "larcore/Geometry/Geometry.h"
#include "larreco/Calorimetry/CalorimetryAlg.h"
#include "lardataobj/RecoBase/Hit.h" 

//C++ Includes 
#include <iostream>
#include <vector> 

//Root Includes 
#include "TVector3.h"

namespace ShowerRecoTools{

  class ShowerStandardCalodEdx:IShowerTool {

  public:

    ShowerStandardCalodEdx(const fhicl::ParameterSet& pset);

    ~ShowerStandardCalodEdx(); 

    //Generic Direction Finder
    int CalculateElement(const art::Ptr<recob::PFParticle>& pfparticle,
			 art::Event& Event,
			 reco::shower::ShowerElementHolder& ShowerEleHolder
			 ) override;

  private:

    // Define standard art tool interface
    void configure(const fhicl::ParameterSet& pset) override;

    art::ServiceHandle<geo::Geometry> fGeom;
    calo::CalorimetryAlg fCalorimetryAlg;
    double fdEdxTrackLength;
    bool fMaxHitPlane;
    bool fMissFirstPoint;


  };


  ShowerStandardCalodEdx::ShowerStandardCalodEdx(const fhicl::ParameterSet& pset):
    fCalorimetryAlg(pset.get<fhicl::ParameterSet>("CalorimetryAlg"))
  {
    configure(pset);
  }

  ShowerStandardCalodEdx::~ShowerStandardCalodEdx()
  {
  }

  void ShowerStandardCalodEdx::configure(const fhicl::ParameterSet& pset)
  {
    fdEdxTrackLength = pset.get<float>("dEdxTrackLength");
    fMaxHitPlane     = pset.get<bool> ("MaxHitPlane");
    fMissFirstPoint  = pset.get<bool> ("MissFirstPoint"); 
  }

  int ShowerStandardCalodEdx::CalculateElement(const art::Ptr<recob::PFParticle>& pfparticle,
					       art::Event& Event,
					       reco::shower::ShowerElementHolder& ShowerEleHolder
					       ){


    // Shower dEdx calculation
    std::cout << "hello world dEdx (in track finder)" << std::endl;

    if(!ShowerEleHolder.CheckElement("ShowerStartPosition")){
      mf::LogError("ShowerStandardCalodEdx") << "Start position not set, returning "<< std::endl;
      return 1;
    }
    if(!ShowerEleHolder.CheckElement("InitialTrackHits")){
      mf::LogError("ShowerStandardCalodEdx") << "Initial Track Hits not set returning"<< std::endl;
      return 1;
    }
    if(!ShowerEleHolder.CheckElement("ShowerDirection")){
      mf::LogError("ShowerStandardCalodEdx") << "Shower Direction not set"<< std::endl;
      return 1;
    }

    //Get the initial track hits
    std::vector<art::Ptr<recob::Hit> > trackhits;
    ShowerEleHolder.GetElement("InitialTrackHits",trackhits);

    if(trackhits.size() == 0){
      mf::LogWarning("ShowerStandardCalodEdx") << "Not Hits in the initial track" << std::endl;
      return 0;
    }

    TVector3 ShowerStartPosition = {-999,-999,-999};
    ShowerEleHolder.GetElement("ShowerStartPosition",ShowerStartPosition);
    geo::TPCID vtxTPC = fGeom->FindTPCAtPosition(ShowerStartPosition); 

    std::vector<double> dEdxVec;
    //std::map<geo::PlaneID, std::vector<art::Ptr<recob::Hit> > > trackhits;
    std::vector<std::vector<art::Ptr<recob::Hit> > > trackHits;

    TVector3 showerDir = {-999,-999,-999};
    ShowerEleHolder.GetElement("ShowerDirection",showerDir);

    unsigned int numPlanes = fGeom->Nplanes();

    //dEdx.resize(numPlanes);
    trackHits.resize(numPlanes);

    // TODO replace trackhits look with loop over associated hits with track
    for (unsigned int hitIt=0; hitIt<trackhits.size(); ++hitIt) {
      art::Ptr<recob::Hit> hit = trackhits.at(hitIt);
      geo::PlaneID hitWire = hit->WireID();
      geo::TPCID TPC = hitWire.asTPCID();
      if (TPC==vtxTPC){
	(trackHits.at(hitWire.Plane)).push_back(hit);
      }
    }

    int bestPlane = -999;
    int bestHitsPlane = 0;
    int bestPlaneHits = 0;
    double minPitch = 999;

    std::vector<art::Ptr<recob::Hit> > trackPlaneHits;
    for (unsigned int plane=0; plane<numPlanes; ++plane) {
      trackPlaneHits = trackHits.at(plane);
      std::cout<<"Plane "<<plane<<" with trackhits "<<trackPlaneHits.size()<<std::endl; 
      if (trackPlaneHits.size()){
	double fdEdx = -999;
	double totQ  = 0;
	double avgT  = 0;
	double pitch = 0;
	double wirepitch = fGeom->WirePitch(trackPlaneHits.at(0)->WireID().planeID());
	double angleToVert = fGeom->WireAngleToVertical(fGeom->Plane(plane).View(),trackPlaneHits[0]->WireID().planeID()) - 0.5*TMath::Pi();
	double cosgamma = std::abs(sin(angleToVert)*showerDir.Y()+cos(angleToVert)*showerDir.Z());
	if (cosgamma>0) pitch = wirepitch/cosgamma;
	if (pitch){
	  int nhits = 0;
	  std::vector<float> vQ;
	  int w0 = trackPlaneHits.at(0)->WireID().Wire;
	  //std::cout<<"Wire 0: "<<w0<<std::endl;
	  for (auto const& hit: trackPlaneHits){

	    int w1 = hit->WireID().Wire;	    
	    if (fMissFirstPoint && w0==w1){
	      continue;
	    }

	    if (std::abs((w1-w0)*pitch)<fdEdxTrackLength){ 
	      //std::cout<<"Wire: "<<w1<<" with dQdx: "<<hit->Integral()/pitch<<" and peak time: "<<hit->PeakTime()<<std::endl;
	      vQ.push_back(hit->Integral());
	      totQ += hit->Integral();
	      avgT+= hit->PeakTime();
	      ++nhits;
	    }
	  }

	  if (totQ) {
	    if (pitch<minPitch){
	      //std::cout<<"New best plane: "<<plane<<" with pitch: "<<pitch<<std::endl;
	      minPitch  = pitch;
	      bestPlane = plane;
	    }

	    double dQdx = TMath::Median(vQ.size(), &vQ[0])/pitch;
	    //	    int expHits = (int)fdEdxTrackLength/pitch;	    
	    fdEdx = fCalorimetryAlg.dEdx_AREA(dQdx, avgT/nhits, trackPlaneHits[0]->WireID().Plane);

	    std::cout<<"dQdx: "<<dQdx<<" dEdx: "<<fdEdx<<" avgT/nhits: "<< avgT/nhits <<" numHits: "<<nhits<< " pitch: "<<pitch<<" plane: "<<trackPlaneHits[0]->WireID().Plane<<std::endl;

	    if (isinf(fdEdx)) { //TODO add error message logger
	      fdEdx=-999;
	    };

	    if (nhits > bestPlaneHits || ((nhits==bestPlaneHits) && (pitch<minPitch))){
	      bestHitsPlane = plane;
	      bestPlaneHits = nhits;
	    }
	  }
	  //std::cout<<"dEdx: "<<fdEdx<<std::endl;
	  dEdxVec.push_back(fdEdx);  
	} else { // if not (pitch)
	  dEdxVec.push_back(-999);
	};
      } else { // if not (trackPlaneHits.size())
	dEdxVec.push_back(-999);
      }
      trackPlaneHits.clear();
    } //end loop over planes 

    //To do
    std::vector<double> dEdxVecErr = {-999,-999,-999};

    ShowerEleHolder.SetElement(dEdxVec,dEdxVecErr,"ShowerdEdx");

    //Set The best plane 
    if (fMaxHitPlane){
      bestPlane=bestHitsPlane;
    }

    if (bestPlane==-999){
      //mf::LogError("ShowerTrackFinder") << "No best plane set "<< std::endl;
      throw cet::exception("ShowerTrackFinderEMShower") << "No best plane set";
      return 1;
    } else {
      ShowerEleHolder.SetElement(bestPlane,"ShowerBestPlane");
      //ShowerEleHolder.SetBestPlane(bestHitsPlane);
    }
    std::cout<<"Best Plane: "<<bestPlane<<" and plane with most hits: "<<bestHitsPlane<<std::endl;
    std::cout<<"dEdx: "<<dEdxVec[bestPlane]<<std::endl;

    return 0;
  }
}

DEFINE_ART_CLASS_TOOL(ShowerRecoTools::ShowerStandardCalodEdx)

