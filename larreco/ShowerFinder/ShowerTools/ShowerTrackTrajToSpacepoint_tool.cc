//############################################################################
//### Name:        ShowerTrackTrajToSpacepoint                             ###
//### Author:      Dominic Barker                                          ###
//### Date:        01.10.19                                                ###
//### Description: Tool to associate the initial track trajectory points   ###
//###              to the spacepoints                                      ###
//############################################################################

#include "larreco/ShowerFinder/ShowerTools/IShowerTool.h"

//Framework Includes
#include "art/Utilities/ToolMacros.h"
#include "art/Utilities/make_tool.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "cetlib_except/exception.h"
#include "canvas/Persistency/Common/Ptr.h"
#include "canvas/Persistency/Common/FindOneP.h"

//LArSoft Includes
#include "larcore/Geometry/Geometry.h"
#include "lardataobj/RecoBase/PFParticle.h"
#include "lardataobj/RecoBase/Track.h"
#include "lardataobj/RecoBase/SpacePoint.h"

namespace ShowerRecoTools {


  class ShowerTrackTrajToSpacepoint: public IShowerTool {

  public:

    ShowerTrackTrajToSpacepoint(const fhicl::ParameterSet& pset);

    ~ShowerTrackTrajToSpacepoint();

    //Match trajectory points to the spacepoints
    int CalculateElement(const art::Ptr<recob::PFParticle>& pfparticle,
			 art::Event& Event,
			 reco::shower::ShowerElementHolder& ShowerEleHolder
			 ) override;

  private:

    float fMaxDist; //Max distance that a spacepoint can be from a trajectory
                    //point to be matched
    art::InputTag fPFParticleModuleLabel;
    
    std::string fInitialTrackSpacePointsOuputLabel;
    std::string fInitialTrackHitsOuputLabel;
    std::string fInitialTrackInputTag;
    std::string fShowerStartPositionInputTag;
    std::string fInitialTrackSpacePointsInputTag;

  };


  ShowerTrackTrajToSpacepoint::ShowerTrackTrajToSpacepoint(const fhicl::ParameterSet& pset)
    : IShowerTool(pset.get<fhicl::ParameterSet>("BaseTools")),
      fMaxDist(pset.get<float>("MaxDist")),
      fPFParticleModuleLabel(pset.get<art::InputTag>("PFParticleModuleLabel")),
      fInitialTrackSpacePointsOuputLabel(pset.get<std::string>("InitialTrackSpacePointsOuputLabel")),
      fInitialTrackHitsOuputLabel(pset.get<std::string>("InitialTrackHitsOuputLabel")),
      fInitialTrackInputTag(pset.get<std::string>("InitialTrackInputTag")),
      fShowerStartPositionInputTag(pset.get<std::string>("ShowerStartPositionInputTag")),
      fInitialTrackSpacePointsInputTag(pset.get<std::string>("InitialTrackSpacePointsInputTag"))
  {
  }

  ShowerTrackTrajToSpacepoint::~ShowerTrackTrajToSpacepoint()
  {
  }

  int ShowerTrackTrajToSpacepoint::CalculateElement(const art::Ptr<recob::PFParticle>& pfparticle,
						    art::Event& Event,
						    reco::shower::ShowerElementHolder& ShowerEleHolder){
    
    //Check the Track has been defined
    if(!ShowerEleHolder.CheckElement(fInitialTrackInputTag)){
      mf::LogError("ShowerTrackTrajToSpacepoint") << "Initial track not set"<< std::endl;
      return 0;
    }

    //Check the start position is set.
    if(!ShowerEleHolder.CheckElement(fShowerStartPositionInputTag)){
      mf::LogError("ShowerTrackTrajToSpacepoint") << "Start position not set, returning "<< std::endl;
      return 0;
    }


    //Check the Track Hits has been defined
    if(!ShowerEleHolder.CheckElement(fInitialTrackSpacePointsInputTag)){
      mf::LogError("ShowerTrackTrajToSpacepoint") << "Initial track spacepoints not set"<< std::endl;
      return 0;
    }

    //Get the start poistion
    TVector3 ShowerStartPosition = {-999,-999,-999};
    ShowerEleHolder.GetElement(fShowerStartPositionInputTag,ShowerStartPosition);


    //Get the initial track hits.
    std::vector<art::Ptr<recob::SpacePoint> > intitaltrack_sp;
    ShowerEleHolder.GetElement(fInitialTrackSpacePointsInputTag,intitaltrack_sp);

    //Get the track
    recob::Track InitialTrack;
    ShowerEleHolder.GetElement(fInitialTrackInputTag,InitialTrack);

    std::vector<art::Ptr<recob::SpacePoint> > new_intitaltrack_sp;
    //Loop over the trajectory points
    for(unsigned int traj=0; traj< InitialTrack.NumberTrajectoryPoints(); ++traj){

      //ignore bogus info.
      auto flags = InitialTrack.FlagsAtPoint(traj);
      if(flags.isSet(recob::TrajectoryPointFlagTraits::NoPoint))
      {continue;}

      geo::Point_t TrajPositionPoint = InitialTrack.LocationAtPoint(traj);
      TVector3 TrajPosition = {TrajPositionPoint.X(),TrajPositionPoint.Y(),TrajPositionPoint.Z()};

      geo::Point_t TrajPositionStartPoint = InitialTrack.LocationAtPoint(0);
      TVector3 TrajPositionStart = {TrajPositionStartPoint.X(),TrajPositionStartPoint.Y(),TrajPositionStartPoint.Z()};


      //Ignore values with 0 mag from the start position
      if((TrajPosition - TrajPositionStart).Mag() == 0){continue;}
      if((TrajPosition - ShowerStartPosition).Mag() == 0){continue;}

      float MinDist = 9999;
      unsigned int index = 999;
      for(unsigned int sp=0; sp<intitaltrack_sp.size();++sp){
        //Find the spacepoint closest to the trajectory point.
        art::Ptr<recob::SpacePoint> spacepoint = intitaltrack_sp[sp];
        TVector3 pos = IShowerTool::GetTRACSAlg().SpacePointPosition(spacepoint) - TrajPosition;
        if(pos.Mag() < MinDist && pos.Mag()< fMaxDist){
          MinDist = pos.Mag();
          index = sp;
        }
      }

      if(index == 999){continue;}
      //Add the spacepoint to the track spacepoints.
      new_intitaltrack_sp.push_back(intitaltrack_sp[index]);

      //Delete the spacepoint so it can not be used again.
      intitaltrack_sp.erase(intitaltrack_sp.begin() + index);
    }


    // Get the spacepoints
    art::Handle<std::vector<recob::SpacePoint> > spHandle;
    if (!Event.getByLabel(fPFParticleModuleLabel, spHandle)){
      throw cet::exception("ShowerTrackTrajToSpacepoint") << "Could not configure the spacepoint handle. Something is configured incorrectly. Stopping";
      return 1;
    }

    // Get the hits associated with the space points
    art::FindOneP<recob::Hit> fohsp(spHandle, Event, fPFParticleModuleLabel);
    if(!fohsp.isValid()){
      throw cet::exception("ShowerTrackTrajToSpacepoint") << "Spacepoint and hit association not valid. Stopping.";
      return 1;
    }

    //Save the corresponding hits
    std::vector<art::Ptr<recob::Hit> > trackHits;
    for(auto const& spacePoint: new_intitaltrack_sp){
      //Get the hits
      const art::Ptr<recob::Hit> hit = fohsp.at(spacePoint.key());
      trackHits.push_back(hit);
    }

    //Save the spacepoints.
    ShowerEleHolder.SetElement(new_intitaltrack_sp,fInitialTrackSpacePointsOuputLabel);
    ShowerEleHolder.SetElement(trackHits,fInitialTrackHitsOuputLabel);

    return 0;
  }

}

DEFINE_ART_CLASS_TOOL(ShowerRecoTools::ShowerTrackTrajToSpacepoint)

