/**
 *  @file   PCASeedFinderAlg.h
 *
 *  @brief  This is an algorithm for finding recob::Seed objects in 3D clusters
 *
 */
#ifndef PCASeedFinderAlg_h
#define PCASeedFinderAlg_h

// Framework Includes
#include "fhiclcpp/fwd.h"

// LArSoft includes
#include "larreco/RecoAlg/Cluster3DAlgs/SeedFinderAlgBase.h"
#include "larreco/RecoAlg/Cluster3DAlgs/PrincipalComponentsAlg.h"
#include "larreco/RecoAlg/Cluster3DAlgs/Cluster3D.h"
namespace geo { class Geometry; }

// ROOT includes
class TVector3;

//------------------------------------------------------------------------------------------------------------------------------------------

namespace lar_cluster3d
{

/**
 *  @brief  PCASeedFinderAlg class
 */
class PCASeedFinderAlg : virtual public SeedFinderAlgBase
{
public:
    /**
     *  @brief  Constructor
     *
     *  @param  pset
     */
    PCASeedFinderAlg(fhicl::ParameterSet const &pset);

    /**
     *  @brief  Destructor
     */
    virtual ~PCASeedFinderAlg();

    /**
     *  @brief a handler for the case where the algorithm control parameters are to be reset
     */
    virtual void reconfigure(fhicl::ParameterSet const &pset);

    /**
     *  @brief Given the list of hits this will search for candidate Seed objects and return them
     */
    virtual bool findTrackSeeds(reco::HitPairListPtr&      hitPairListPtr,
                                reco::PrincipalComponents& inputPCA,
                                SeedHitPairListPairVec&    seedHitMap) const;

private:

    /**
     *  @brief Separate function to find hits at the ends of the input hits
     */
    bool getHitsAtEnd(reco::HitPairListPtr& hit3DList, reco::PrincipalComponents& seedPca) const;

    void LineFit2DHits(const reco::HitPairListPtr& hitList, double XOrigin, TVector3& Pos, TVector3& Dir, double& ChiDOF) const;

    geo::Geometry const*                   m_geometry;         // pointer to the Geometry service
    //    const detinfo::DetectorProperties*    m_detector;         // Pointer to the detector properties

    double                                 m_gapDistance;      ///<
    size_t                                 m_numSeed2DHits;    ///<
    double                                 m_minAllowedCosAng; ///< The minimum cos(ang) between input and seed axes

    PrincipalComponentsAlg                 m_pcaAlg;           // For running Principal Components Analysis
};

} // namespace lar_cluster3d
#endif
