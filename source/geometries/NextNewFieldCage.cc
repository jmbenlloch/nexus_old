// ----------------------------------------------------------------------------
//  $Id$
//
//  Authors: <miquel.nebot@ific.uv.es>
//  Created: 12 Sept 2013
//  
//  Copyright (c) 2013 NEXT Collaboration
// ---------------------------------------------------------------------------- 

#include "NextNewFieldCage.h"
#include "MaterialsList.h"
#include <G4GenericMessenger.hh>
#include "UniformElectricDriftField.h"
#include "OpticalMaterialProperties.h"
#include "IonizationSD.h"
#include "XenonGasProperties.h"

#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>
#include <G4Material.hh>
#include <G4LogicalVolume.hh>
#include <G4Tubs.hh>
#include <G4OpticalSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4NistManager.hh>
#include <G4UserLimits.hh>
#include <G4SDManager.hh>
#include <G4RunManager.hh>

namespace nexus {

  NextNewFieldCage::NextNewFieldCage(): 
    BaseGeometry(),
    // Field cage dimensions
    // _active_diam (48.2 * cm),
    _drift_length (539.45 * mm),//tube_length - buffer -cathode
    // _trk_displ (13.36 * cm),
    //  _ener_displ (9.5 * cm),
    _el_gap_length (.5 * cm),
    _grid_thickness (.1 * mm), //it's just fake dielectric
    _el_tot_zone (24.1 * mm),
    _el_grid_transparency (.88),
    _gate_transparency (.76),
    _cathode_grid_transparency (.98),
    _cage_tot_length (58.9 * cm),   // Drift volume (51cm) + Buffer (7cm) 
    _tube_length (611.95*mm),//554-12.05+60+10// (56.49 * cm)= 58.9??? -2.41
    _tube_in_diam (48. * cm),  //=_vessel_in_rad -ics - _tube_thickness  
    _tube_thickness (2.0 * cm),    
    _reflector_thickness (.5 * cm),
    _el_gap_z_pos ( 277 * mm)

  {
    /// Messenger
    _msg = new G4GenericMessenger(this, "/Geometry/NextNew/", 
			     "Control commands of geometry NextNew.");
    _msg->DeclareProperty("field_cage_vis", _visibility, 
			  "Field Cage Visibility");

    
  // Boolean-type properties (true or false)
  _msg->DeclareProperty("elfield", _elfield,
			"True if the EL field is on (full simulation), false if it's not (parametrized simulation.");

    G4GenericMessenger::Command& step_cmd = 
      _msg->DeclareProperty("max_step_size", _max_step_size, 
			    "Maximum Step Size");
    step_cmd.SetUnitCategory("Length");
    step_cmd.SetParameterName("max_step_size", false);
    step_cmd.SetRange("max_step_size>0.");
  }

  void NextNewFieldCage::SetLogicalVolume(G4LogicalVolume* mother_logic)
  {
    _mother_logic = mother_logic;
    // Reading material
    _gas = _mother_logic->GetMaterial();
    _pressure =    _gas->GetPressure();
    _temperature = _gas->GetTemperature();
  }

  void NextNewFieldCage::Construct()
  {
    // EL Region
    BuildELRegion();
    // Cathode Grid
    BuildCathodeGrid();
    // ACTIVE region
    BuildActive(); 

    //////FIELD CAGE//////
    _tube_z_pos =  _el_gap_z_pos -_el_tot_zone/2. -_tube_length/2.;
    //(160.*cm/2. + 8.7*cm - _tube_length/2.);  //Vessel_body_len (160)+ Ener_zone displ(8.7)-
    G4Tubs* field_cage_solid =
      new G4Tubs("FIELD_CAGE", _tube_in_diam/2.,
		 _tube_in_diam/2. + _tube_thickness, _tube_length/2., 0, twopi);
    _hdpe = MaterialsList::HDPE();
    G4LogicalVolume* field_cage_logic = 
      new G4LogicalVolume(field_cage_solid, _hdpe, "FIELD_CAGE");
    G4PVPlacement* field_cage_physi = 
      new G4PVPlacement(0, G4ThreeVector(0., 0., _tube_z_pos), 
			field_cage_logic, "FIELD_CAGE", _mother_logic, 
			false, 0,true);
    //Internal reflector
    G4Tubs* reflector_solid = 
      new G4Tubs("FC_REFLECTOR", _tube_in_diam/2.-_reflector_thickness, 
		 _tube_in_diam/2., _tube_length/2., 0, twopi);
    G4Material* teflon = 
      G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON");
    G4LogicalVolume* reflector_logic = 
      new G4LogicalVolume(reflector_solid, teflon, "FC_REFLECTOR");
    G4PVPlacement* reflector_physi = 
      new G4PVPlacement(0, G4ThreeVector(0., 0., _tube_z_pos), 
			reflector_logic, "FC_REFLECTOR", _mother_logic, 
			false, 0,true);

    // TPB coating
    G4Tubs* tpb_solid =
      new G4Tubs("REFLECTOR_TPB", _tube_in_diam/2.-_reflector_thickness,
		 _tube_in_diam/2.-_reflector_thickness + 1.*micrometer,
		 _tube_length/2., 0, twopi);
    G4Material* tpb = MaterialsList::TPB();
    tpb->SetMaterialPropertiesTable(OpticalMaterialProperties::TPB());
    G4LogicalVolume* tpb_logic = 
      new G4LogicalVolume(tpb_solid, tpb, "REFLECTOR_TPB");
    G4PVPlacement* tpb_physi = 
      new G4PVPlacement(0, G4ThreeVector(0., 0., 0.), 
			tpb_logic, "REFLECTOR_TPB", reflector_logic, 
			false, 0,true);

    /// OPTICAL SURFACE PROPERTIES    ////////
    G4OpticalSurface* reflector_opt_surf = new G4OpticalSurface("FC_REFLECTOR");
    reflector_opt_surf->SetType(dielectric_metal);
    reflector_opt_surf->SetModel(unified);
    reflector_opt_surf->SetFinish(ground);
    reflector_opt_surf->SetSigmaAlpha(0.1);
    reflector_opt_surf->SetMaterialPropertiesTable(OpticalMaterialProperties::PTFE_with_TPB());
    new G4LogicalSkinSurface("FC_REFLECTOR", reflector_logic, 
			     reflector_opt_surf);

    /// SETTING VISIBILITIES   //////////
    // if (_visibility) {
    G4VisAttributes hdpe_col(G4Colour(.6, .8, .79));
    //hdpe_col.SetForceSolid(true);    
    //field_cage_logic->SetVisAttributes(hdpe_col);
    G4VisAttributes teflon_col(G4Colour(0., 0., .7));
    //teflon_col.SetForceSolid(true);
    reflector_logic->SetVisAttributes(teflon_col);
    G4VisAttributes tpb_col(G4Colour(1., 0., 0.));
    //   tpb_col.SetForceSolid(true);
    tpb_logic->SetVisAttributes(tpb_col);
    // }
    // else {
    field_cage_logic->SetVisAttributes(G4VisAttributes::Invisible);
    //   reflector_logic->SetVisAttributes(G4VisAttributes::Invisible);
    // }

    /// VERTEX GENERATORS   //////////
    _field_cage_gen  = 
      new CylinderPointSampler(_tube_in_diam/2., _tube_length, _tube_thickness,
			       0., G4ThreeVector (0., 0., _tube_z_pos));

    _reflector_gen  = 
      new CylinderPointSampler(_tube_in_diam/2.- _reflector_thickness, 
			       _tube_length, _reflector_thickness,
			       0., G4ThreeVector (0., 0., _tube_z_pos));
    G4double z = _el_gap_z_pos - _el_gap_length/2. + .5*mm;
    //    G4cout << "Zeta pos: "<< z << G4endl;
    CalculateELTableVertices(_tube_in_diam/2., 5.*mm, z);
    // G4cout << "Points: "<< _table_vertices.size() << G4endl;
    // for (int i=0; i<_table_vertices.size(); i++){
    //   G4cout << _table_vertices[i][0]<< ", "<< _table_vertices[i][1] << ",  " << _table_vertices[i][2] << G4endl; 
    // }
  }

  NextNewFieldCage::~NextNewFieldCage()
  {
    delete _field_cage_gen;
    delete _reflector_gen;
    delete _active_gen;
   }
  
  G4ThreeVector NextNewFieldCage::GetPosition() const
  {
    return G4ThreeVector (0., 0., _tube_z_pos);
  }

  G4ThreeVector NextNewFieldCage::GenerateVertex(const G4String& region) const
  {
    G4ThreeVector vertex(0., 0., 0.);
    if (region == "FIELD_CAGE") {
      vertex = _field_cage_gen->GenerateVertex(TUBE_VOLUME);
    }
    else if (region == "REFLECTOR") {
      vertex = _reflector_gen->GenerateVertex(TUBE_VOLUME);
    }
    // Active region
    else if (region == "ACTIVE") {
      vertex = _active_gen->GenerateVertex(INSIDE);
    } 
    else if (region == "EL_TABLE") {  
      _idx_table++;	
      if(_idx_table>=_table_vertices.size()){
    	G4cout<<"[NextNewFieldCage::GenerateVertex()] Aborting the run,"
	      << " last event reached ..."<<G4endl;
    	G4RunManager::GetRunManager()->AbortRun();
      }
      if(_idx_table<=_table_vertices.size()){   
    	return _table_vertices[_idx_table-1];
      }
    } 
    // //EL GAP
    // //Cathode grid
 
    return vertex;
  }

void NextNewFieldCage::CalculateELTableVertices(G4double radius, 
				       G4double binning, G4double z)		
{     
    G4ThreeVector position;

    ///Squared pitch
    position[2] = z;

    G4int imax = floor(2*radius/binning);

    for (int i=0; i<imax; i++){
      position[0] = -radius + i*binning;
      for (int j=0; j<imax; j++){
	position[1] = -radius + j*binning;
	if (sqrt(position[0]*position[0]+position[1]*position[1])< radius){
	  _table_vertices.push_back(position);	  
	} else {
	  continue;
	}
      }
    }
}


  void NextNewFieldCage::BuildELRegion()
  {
    ///// EL GAP /////
    G4double el_gap_diam = _tube_in_diam;  
    G4Tubs* el_gap_solid = 
      new G4Tubs("EL_GAP", 0., el_gap_diam/2., _el_gap_length/2., 0, twopi);   
    G4LogicalVolume* el_gap_logic = 
      new G4LogicalVolume(el_gap_solid, _gas, "EL_GAP");  
    G4PVPlacement* el_gap_physi = 
      new G4PVPlacement(0, G4ThreeVector(0., 0., _el_gap_z_pos), el_gap_logic,
			"EL_GAP", _mother_logic, false, 0,true);
   

    if (_elfield) {
      // Define EL electric field
      UniformElectricDriftField* el_field = new UniformElectricDriftField();
      el_field->SetCathodePosition(_el_gap_z_pos - _el_gap_length/2.);
      el_field->SetAnodePosition  (_el_gap_z_pos + _el_gap_length/2.);
      el_field->SetDriftVelocity(2.5 * mm/microsecond);
      el_field->SetTransverseDiffusion(1. * mm/sqrt(cm));
      el_field->SetLongitudinalDiffusion(.5 * mm/sqrt(cm));
      XenonGasProperties xgp(_pressure, _temperature);
      el_field->SetLightYield(xgp.ELLightYield(20*kilovolt/cm));
      G4Region* el_region = new G4Region("EL_REGION");
      el_region->SetUserInformation(el_field);
      el_region->AddRootLogicalVolume(el_gap_logic);
    }

    ///// EL GRIDS /////
    G4Material* fgrid_mat = MaterialsList::FakeDielectric(_gas, "el_grid_anode_mat");
    fgrid_mat->SetMaterialPropertiesTable(OpticalMaterialProperties::FakeGrid(_pressure, _temperature, _el_grid_transparency, _grid_thickness));

    G4Material* fgate_mat = MaterialsList::FakeDielectric(_gas, "el_grid_gate_mat");
    fgate_mat->SetMaterialPropertiesTable(OpticalMaterialProperties::FakeGrid(_pressure, _temperature, _gate_transparency, _grid_thickness));
    
    // Dimensions & position: the grids are simulated inside the EL gap. Their thickness 
    // is symbolic.
    G4double grid_diam = _tube_in_diam; // _active_diam;
    G4double poszInner =  - _el_gap_length/2. + _grid_thickness/2.;
    G4double poszOuter =  _el_gap_length/2. - _grid_thickness/2.;
   
    G4Tubs* diel_grid_solid = 
      new G4Tubs("EL_GRID", 0., grid_diam/2., _grid_thickness/2., 0, twopi);

    G4LogicalVolume* gate_logic = 
      new G4LogicalVolume(diel_grid_solid, fgate_mat, "EL_GRID_GATE");
    G4PVPlacement* gate_physi = 
      new G4PVPlacement(0, G4ThreeVector(0., 0., poszInner), gate_logic, 
			"EL_GRID_GATE", el_gap_logic, false, 0,true);

    G4LogicalVolume* anode_logic = 
      new G4LogicalVolume(diel_grid_solid, fgrid_mat, "EL_GRID_ANODE");
    G4PVPlacement* anode_physi = 
      new G4PVPlacement(0, G4ThreeVector(0., 0., poszOuter), anode_logic, 
			"EL_GRID_ANODE", el_gap_logic, false, 1, true);
    
    /// Visibilities
  //   if (_grids_visibility) {
    G4VisAttributes grid_col(G4Colour(0.5, 0.5, .5));
    // grid_col.SetForceSolid(true);
    el_gap_logic->SetVisAttributes(grid_col);
  //     // grids are white
  //   }
  //   else {
  //     el_gap_logic->SetVisAttributes(G4VisAttributes::Invisible);
  //     diel_grid_logic->SetVisAttributes(G4VisAttributes::Invisible);
  //   }
  }

  void NextNewFieldCage::BuildCathodeGrid()
  {
    ///// CATHODE ////// 
    G4Material* fgrid_mat = MaterialsList::FakeDielectric(_gas, "cath_grid_mat");
    fgrid_mat->SetMaterialPropertiesTable(OpticalMaterialProperties::FakeGrid(_pressure, _temperature, _cathode_grid_transparency, _grid_thickness));
    // Dimensions & position
    G4double grid_diam = _tube_in_diam - 2*_reflector_thickness;
    G4double posz = - _el_gap_z_pos;//_el_grid_ref_z - _grid_thickn - _active_length;
  //   //G4cout << G4endl << "Cathode Grid posz: " << posz/cm << G4endl;
    // Building the grid
    G4Tubs* diel_grid_solid = 
      new G4Tubs("CATH_GRID", 0., grid_diam/2., _grid_thickness/2., 0, twopi);
    G4LogicalVolume* diel_grid_logic = 
      new G4LogicalVolume(diel_grid_solid, fgrid_mat, "CATHODE_GRID");
    G4PVPlacement* diel_grid_physi = 
      new G4PVPlacement(0, G4ThreeVector(0.,0.,posz), diel_grid_logic, 
			"CATHODE_GRID", _mother_logic, false, 0,true);
    /// Visibilities
    G4VisAttributes grid_col(G4Colour(0.5, 0.5, .5));
    //grid_col.SetForceSolid(true);
    diel_grid_logic->SetVisAttributes(grid_col);
    //  // Grid is white
  //   if (!_grids_visibility) {
  //     diel_grid_logic->SetVisAttributes(G4VisAttributes::Invisible);
  //   }
  }



  void NextNewFieldCage::BuildActive()
  {
    G4double active_posz = _el_gap_z_pos -_el_tot_zone/2. - _drift_length/2.;
    G4Tubs* active_solid = 
      new G4Tubs("ACTIVE",  0., _tube_in_diam/2.-_reflector_thickness, 
		 _drift_length/2., 0, twopi);
    G4LogicalVolume* active_logic = 
      new G4LogicalVolume(active_solid, _gas, "ACTIVE");
    G4PVPlacement* active_physi = 
      new G4PVPlacement(0, G4ThreeVector(0., 0., active_posz), active_logic, 
			"ACTIVE", _mother_logic, false, 0,true);
    // Limit the step size in this volume for better tracking precision
    active_logic->SetUserLimits(new G4UserLimits(_max_step_size));
    // Set the volume as an ionization sensitive detector
    IonizationSD* ionisd = new IonizationSD("/NEXTNEW/ACTIVE");
    active_logic->SetSensitiveDetector(ionisd);
    G4SDManager::GetSDMpointer()->AddNewDetector(ionisd);
    // Define a drift field for this volume
    UniformElectricDriftField* field = new UniformElectricDriftField();
    field->SetCathodePosition(active_posz - _drift_length/2.);
    field->SetAnodePosition(active_posz + _drift_length/2.);
    field->SetDriftVelocity(1. * mm/microsecond);
    field->SetTransverseDiffusion(1. * mm/sqrt(cm));
    field->SetLongitudinalDiffusion(.5 * mm/sqrt(cm));  
    G4Region* drift_region = new G4Region("DRIFT");
    drift_region->SetUserInformation(field);
    drift_region->AddRootLogicalVolume(active_logic);
  
    /// Visibilities
    active_logic->SetVisAttributes(G4VisAttributes::Invisible);
    // VERTEX GENERATOR
    _active_gen = 
      new CylinderPointSampler(0., _drift_length,
			       _tube_in_diam/2.-_reflector_thickness,
			       0., G4ThreeVector (0., 0., active_posz));
  }

}//end namespace nexus

