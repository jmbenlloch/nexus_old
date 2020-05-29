// ----------------------------------------------------------------------------
///  \file
///  \brief
///
///  \author   <justo.martin-albo@ific.uv.es>, <jmunoz@ific.uv.es>
///  \date     21 Nov 2011
///  \version  $Id$
///
///  Copyright (c) 2011 NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef _NEXT100___
#define _NEXT100___

#include "BaseGeometry.h"
#include "Next100Shielding.h"
#include "Next100Vessel.h"
#include "Next100Ics.h"
#include "Next100InnerElements.h"

class G4LogicalVolume;
class G4GenericMessenger;

namespace nexus {class BoxPointSampler;}


namespace nexus {


  class Next100: public BaseGeometry
  {
  public:
    /// Constructor
    Next100();

    /// Destructor
    ~Next100();

    /// Generate a vertex within a given region of the geometry
    G4ThreeVector GenerateVertex(const G4String& region) const;


  private:
    void BuildLab();
    void Construct();


  private:
    // Detector dimensions
    const G4double lab_size_;          /// Size of the air box containing the detector

    // External diameter of nozzles and y positions
    const G4double nozzle_ext_diam_;
    const G4double up_nozzle_ypos_, central_nozzle_ypos_, down_nozzle_ypos_, bottom_nozzle_ypos_;

    // Pointers to logical volumes
    G4LogicalVolume* lab_logic_;
    G4LogicalVolume* buffer_gas_logic_;

    // Detector parts
    Next100Shielding* shielding_;
    Next100Vessel*    vessel_;
    Next100Ics*       ics_;
    Next100InnerElements* inner_elements_;

    BoxPointSampler* lab_gen_; ///< Vertex generator

    /// Messenger for the definition of control commands
    G4GenericMessenger* msg_;

    /// Specific vertex for AD_HOC region
    G4double specific_vertex_X_;
    G4double specific_vertex_Y_;
    G4double specific_vertex_Z_;

    /// Position of gate in its mother volume
    G4double gate_zpos_in_vessel_;
  };

} // end namespace nexus

#endif
