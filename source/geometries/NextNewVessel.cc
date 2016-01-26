// ----------------------------------------------------------------------------
//  $Id$
//
//  Authors: <miquel.nebot@ific.uv.es>, <jmunoz@ific.uv.es>, <justo.martin-albo@ific.uv.es>
//  Created: 9 Sept 2013
//  
//  Copyright (c) 2013 NEXT Collaboration
// ---------------------------------------------------------------------------- 

#include "NextNewVessel.h"
#include "MaterialsList.h"
#include "OpticalMaterialProperties.h"
#include "Visibilities.h"

#include <G4GenericMessenger.hh>
#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>
#include <G4UnionSolid.hh>
#include <G4Tubs.hh>
#include <G4Sphere.hh>
#include <G4NistManager.hh>
#include <G4Material.hh>
#include <G4Colour.hh>
#include <Randomize.hh>
#include <G4TransportationManager.hh>
#include <G4RotationMatrix.hh>
#include <G4UnitsTable.hh>
#include <G4Transform3D.hh>

#include <CLHEP/Units/SystemOfUnits.h>
#include <CLHEP/Units/PhysicalConstants.h>
#include <stdexcept>

namespace nexus {

  using namespace CLHEP;
  
  NextNewVessel::NextNewVessel():

    BaseGeometry(),
   
    // Body dimensions
    _vessel_in_diam (64.0  * cm),
    _vessel_body_length (87. * cm),//body cylinder(without flanges)
    _vessel_tube_length (122.256 * cm),  //  870 body + 2. *( (50-10)bodyflange + (50-10)endcapflange + 96.28 endcap)
    _vessel_thickness (1.2  * cm),

    // Flange dimensions (one cylinder = 1 on the _vessel_body side + 1 on the _endcap side)
    _flange_out_diam (82.0 * cm),//Flange inner radius = vessel inner radius + vessel thickness
    _flange_length (10.0 * cm), //5for body + 5for endcap
    _flange_z_pos (_vessel_body_length/2.),//435

    // Endcaps dimensions
    _endcap_in_rad (53.2 * cm), //=(66.52/2 - 1.26)
    _endcap_theta (37.2 * deg), //(38.7 * deg)- 1.5º visfit 
    _endcap_thickness (1.26 * cm),
    _endcap_in_z_width (9.448 * cm),// inner sphere to flange (96,28)-visfit (1.8mm)

    // Nozzle dimensions
    _lat_nozzle_in_diam  (30.*mm),
    _lat_nozzle_high (51.*mm),
    _lat_nozzle_thickness (6.1*mm),
    _lat_nozzle_flange_diam (75.*mm),
    _lat_nozzle_flange_high (15.*mm), // This is half the total thickness of the flange+cover

    _up_nozzle_in_diam (60.*mm), 
    _up_nozzle_high (54.3*mm),
    _up_nozzle_thickness (6.51*mm),
    _up_nozzle_flange_diam (130*mm),
    _up_nozzle_flange_high (16.7*mm), // This is half the total thickness of the flange+cover
   
    _endcap_nozzle_in_diam (88.*mm),
    _endcap_nozzle_high (86.*mm),
    _endcap_nozzle_thickness (6.8*mm),
    _endcap_nozzle_flange_diam (165.*mm),
    _endcap_nozzle_flange_high (22.*mm),

    _lat_nozzle_z_pos (120*mm),
    _lat_nozzle_x_pos (_vessel_in_diam/2. +_lat_nozzle_high/2.),
    _up_nozzle_y_pos (_vessel_in_diam/2. +_up_nozzle_high/2.),
    _up_nozzle_z_pos ( 277. *mm),//anode_nozzle_zpos = -cathode_nozzle_zpos
    _endcap_nozzle_z_pos (_vessel_tube_length/2. + _endcap_in_z_width), //bodylegth/2 + 300mm???? 

    _port_tube_diam (10. * mm),
    _port_tube_thickness (1. * mm),
    _port_tube_window_thickn (2. * mm),
    _lat_port_tube_length (53. * mm), 
    _up_port_tube_length (58. * mm), 
    _axial_port_tube_length (727. * mm),


    // Vessel gas
     _pressure(1. * bar),
    _temperature (303 * kelvin),
    _visibility(1),
    _sc_yield(16670. * 1/MeV)

  {
    /// Needed External variables


    // Initializing the geometry navigator (used in vertex generation)
    _geom_navigator = G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();

    /// Messenger
    _msg = new G4GenericMessenger(this, "/Geometry/NextNew/", "Control commands of geometry NextNew.");
    _msg->DeclareProperty("vessel_vis", _visibility, "Vessel Visibility");

    G4GenericMessenger::Command& pressure_cmd = _msg->DeclareProperty("pressure", _pressure, "Xenon pressure");
    pressure_cmd.SetUnitCategory("Pressure");
    pressure_cmd.SetParameterName("pressure", false);
    pressure_cmd.SetRange("pressure>0.");

    new G4UnitDefinition("1/MeV","1/MeV", "1/Energy", 1/MeV);

    G4GenericMessenger::Command& sc_yield_cmd = 
      _msg->DeclareProperty("sc_yield", _sc_yield,
			    "Set scintillation yield for GXe. It is in photons/MeV");
    sc_yield_cmd.SetParameterName("sc_yield", true);
    sc_yield_cmd.SetUnitCategory("1/Energy");
    
    
  }



void NextNewVessel::Construct()
  {
    // Body solid
    G4double vessel_out_diam = _vessel_in_diam + 2*_vessel_thickness;
    G4Tubs* vessel_tube_solid = new G4Tubs("VESSEL_BODY", 0., vessel_out_diam/2.,
					   _vessel_tube_length/2.,0., twopi);
    G4Tubs* vessel_gas_tube_solid = new G4Tubs("VESSEL_GAS_BODY", 0., _vessel_in_diam/2.,
     					       _vessel_tube_length/2.,0., twopi);

    // Endcaps solids
    G4double endcap_out_rad = _endcap_in_rad + _endcap_thickness;
    G4Sphere* vessel_tracking_endcap_solid = new G4Sphere("VESSEL_TRACKING_ENDCAP",
							  0. * cm,  endcap_out_rad,   //radius
							  0. * deg, 360. * deg,       // phi
							  0. * deg, _endcap_theta);   // theta
    G4Sphere* vessel_gas_tracking_endcap_solid = new G4Sphere("VESSEL_GAS_TRACKING_ENDCAP",
							      0. * cm,  _endcap_in_rad,   //radius
							      0. * deg, 360. * deg,       // phi
							      0. * deg, _endcap_theta);   // theta
    G4Sphere* vessel_energy_endcap_solid = new G4Sphere("VESSEL_ENERGY_ENDCAP",
							0. * cm,  endcap_out_rad,     //radius
							0. * deg, 360. * deg,        // phi
							180. * deg - _endcap_theta, _endcap_theta); // theta
    G4Sphere* vessel_gas_energy_endcap_solid = new G4Sphere("VESSEL_GAS_ENERGY_ENDCAP",
							    0. * cm,  _endcap_in_rad,    //radius
							    0. * deg, 360. * deg,        // phi
							    180. * deg - _endcap_theta, _endcap_theta); // theta
    
    // Flange solid
    G4Tubs* vessel_flange_solid = new G4Tubs("VESSEL_FLANGE", vessel_out_diam/2., _flange_out_diam/2.,
    					     _flange_length/2., 0., twopi);
    
    // Nozzle solids
    G4Tubs* lateral_nozzle_tube_solid = 
      new G4Tubs("LAT_TUBE_NOZZLE", 0., _lat_nozzle_in_diam/2. +_lat_nozzle_thickness, _lat_nozzle_high, 0., twopi);
    G4Tubs* lateral_nozzle_flange_solid = 
      new G4Tubs("LAT_NOZZLE_FLANGE", 0., _lat_nozzle_flange_diam/2., _lat_nozzle_flange_high, 0., twopi);
    G4Tubs* lateral_nozzle_gas_solid = 
      new G4Tubs("LAT_NOZZLE_GAS", 0., _lat_nozzle_in_diam/2.-1*mm, _lat_nozzle_high, 0., twopi);

    G4Tubs* up_nozzle_tube_solid =
      new G4Tubs("UP_TUBE_NOZZLE", 0., _up_nozzle_in_diam/2.+_up_nozzle_thickness,_up_nozzle_high, 0., twopi);
    G4Tubs* up_nozzle_flange_solid =
      new G4Tubs("UP_NOZZLE_FLANGE", 0., _up_nozzle_flange_diam/2.,_up_nozzle_flange_high, 0., twopi);
    G4Tubs* up_nozzle_gas_solid =
      new G4Tubs("UP_NOZZLE_GAS", 0., _up_nozzle_in_diam/2.-1*mm,_up_nozzle_high, 0., twopi);
    
    G4Tubs* endcap_nozzle_tube_solid =
      new G4Tubs("ENDCAP_TUBE_NOZZLE", 0., _endcap_nozzle_in_diam/2.+_endcap_nozzle_thickness,_endcap_nozzle_high, 0., twopi);
    G4Tubs* endcap_nozzle_flange_solid =
      new G4Tubs("ENDCAP_NOZZLE_FLANGE", 0., _endcap_nozzle_flange_diam/2.,_endcap_nozzle_flange_high, 0., twopi);
    G4Tubs* endcap_nozzle_gas_solid =
      new G4Tubs("ENDCAP_NOZZLE_GAS", 0., _endcap_nozzle_in_diam/2.-1*mm,_endcap_nozzle_high, 0., twopi);
  

    //// UNIONS ///////
    G4double endcap_z_pos = (_vessel_tube_length/2.)- (_endcap_in_rad -_endcap_in_z_width);
    G4ThreeVector tracking_endcap_pos(0, 0, endcap_z_pos);
    G4ThreeVector energy_endcap_pos(0, 0, -1. * endcap_z_pos);
    G4ThreeVector tracking_flange_pos(0, 0, _flange_z_pos);
    G4ThreeVector energy_flange_pos(0, 0, -1. * _flange_z_pos);
    
    // Body + Tracking endcap
    G4UnionSolid* vessel_solid = new G4UnionSolid("VESSEL", vessel_tube_solid, vessel_tracking_endcap_solid,
                                                  0, tracking_endcap_pos);
    // Body + Tracking endcap + Energy endcap
    vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, vessel_energy_endcap_solid,
                                     0, energy_endcap_pos);
    // // Body + Tracking endcap + Energy endcap + Tracking flange
    vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, vessel_flange_solid,
    				    0,tracking_flange_pos);
    // // Body + Tracking endcap + Energy endcap + Tracking flange + Energy flange 
    vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, vessel_flange_solid,
      				    0, energy_flange_pos);
   
    // building nozzles
    G4UnionSolid* lateral_nozzle_solid = new G4UnionSolid("LAT_NOZZLE", lateral_nozzle_tube_solid,
     							  lateral_nozzle_flange_solid, 0,
    							  G4ThreeVector(0.,0., _lat_nozzle_high + _lat_nozzle_flange_high));
    G4UnionSolid* up_nozzle_solid = new G4UnionSolid("UP_NOZZLE", up_nozzle_tube_solid,
						     up_nozzle_flange_solid, 0,
						     G4ThreeVector(0.,0.,_up_nozzle_high + _up_nozzle_flange_high));    
    G4UnionSolid* endcap_nozzle_solid = new G4UnionSolid("ENDCAP_NOZZLE", endcap_nozzle_tube_solid,
							 endcap_nozzle_flange_solid, 0,
							 G4ThreeVector(0.,0.,_endcap_nozzle_high + _endcap_nozzle_flange_high));    
    //Rotate the lateral nozzles 
    G4RotationMatrix* rot_lat = new G4RotationMatrix();
    rot_lat->rotateY(3*pi/2.);
    //Rotate the upper nozzles to vertical
    G4RotationMatrix* rot_up = new G4RotationMatrix();
    rot_up->rotateX(pi/2.);
    //Rotate the endcap nozzle 
    G4RotationMatrix* rot_endcap = new G4RotationMatrix();
    rot_endcap->rotateY(pi);
    
    // Body + Tracking endcap + Energy endcap + Tracking flange + Energy flange + Lateral nozzles
    // vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, lateral_nozzle_solid,
    //   				    rot_lat, G4ThreeVector(_lat_nozzle_x_pos, 0.,_lat_nozzle_z_pos));
     vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, lateral_nozzle_solid,
      				    rot_lat, G4ThreeVector( _vessel_in_diam/2., 0.,_lat_nozzle_z_pos));

    // vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, lateral_nozzle_solid,
    //  				    rot_lat, G4ThreeVector(_lat_nozzle_x_pos, 0.,-_lat_nozzle_z_pos));
     vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, lateral_nozzle_solid,
     				    rot_lat, G4ThreeVector(_vessel_in_diam/2., 0.,-_lat_nozzle_z_pos));
    
    // Body + Tracking endcap + Energy endcap + Tracking flange + Energy flange + Lateral nozzles + Upper nozzles
    // vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, up_nozzle_solid,
    // 				    rot_up, G4ThreeVector(0.,_up_nozzle_y_pos,_up_nozzle_z_pos));
     vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, up_nozzle_solid,
				     rot_up, G4ThreeVector(0., _vessel_in_diam/2., _up_nozzle_z_pos));
     // vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, up_nozzle_solid,
     //     				    rot_up, G4ThreeVector(0.,_up_nozzle_y_pos,0.));
     vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, up_nozzle_solid,
				     rot_up, G4ThreeVector(0., _vessel_in_diam/2., 0.));
     vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, up_nozzle_solid,
				     rot_up, G4ThreeVector(0.,_up_nozzle_y_pos+3.8*cm, -_up_nozzle_z_pos)); //!!!!!!!
     
    // Body + Tracking endcap + Energy endcap + Tracking flange + Energy flange + Lateral nozzles + Upper nozzles + Endcap nozzles
    vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, endcap_nozzle_solid,
     				    0, G4ThreeVector(0.,0.,_endcap_nozzle_z_pos));
    vessel_solid = new G4UnionSolid("VESSEL", vessel_solid, endcap_nozzle_solid,
      				    rot_endcap, G4ThreeVector(0.,0.,-_endcap_nozzle_z_pos));
   

    ////GAS SOLID //////
    // Body gas + Tracking endcap gas
    G4UnionSolid* vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_tube_solid,
      						      vessel_gas_tracking_endcap_solid, 0, tracking_endcap_pos);
    // Body gas + Tracking endcap gas + Energy endcap gas
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid,
      						      vessel_gas_energy_endcap_solid, 0, energy_endcap_pos);
    // Body gas + Tracking endcap gas + Energy endcap gas + nozzles gas
    // vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, lateral_nozzle_gas_solid, 
    //   					rot_lat, G4ThreeVector(_lat_nozzle_x_pos, 0.,_lat_nozzle_z_pos));
    // vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, lateral_nozzle_gas_solid, 
    //   					rot_lat, G4ThreeVector(_lat_nozzle_x_pos, 0.,-_lat_nozzle_z_pos));
    // vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, up_nozzle_gas_solid, 
    //    					rot_up, G4ThreeVector(0.,_up_nozzle_y_pos,_up_nozzle_z_pos));
    // vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, up_nozzle_gas_solid, 
    //  					rot_up, G4ThreeVector(0.,_up_nozzle_y_pos,0.));
    // vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, up_nozzle_gas_solid, 
    //    					rot_up, G4ThreeVector(0.,_up_nozzle_y_pos,-_up_nozzle_z_pos));
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, lateral_nozzle_gas_solid, 
      					rot_lat, G4ThreeVector(_vessel_in_diam/2., 0.,_lat_nozzle_z_pos));
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, lateral_nozzle_gas_solid, 
      					rot_lat, G4ThreeVector(_vessel_in_diam/2., 0.,-_lat_nozzle_z_pos));
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, up_nozzle_gas_solid, 
       					rot_up, G4ThreeVector(0., _vessel_in_diam/2.,_up_nozzle_z_pos));
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, up_nozzle_gas_solid, 
     					rot_up, G4ThreeVector(0., _vessel_in_diam/2,0.));
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, up_nozzle_gas_solid, 
       					rot_up, G4ThreeVector(0., _vessel_in_diam/2,-_up_nozzle_z_pos));
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, endcap_nozzle_gas_solid, 
       					0, G4ThreeVector(0.,0.,_endcap_nozzle_z_pos));
    vessel_gas_solid = new G4UnionSolid("VESSEL_GAS", vessel_gas_solid, endcap_nozzle_gas_solid, 
      					rot_endcap, G4ThreeVector(0.,0.,-_endcap_nozzle_z_pos));
    

    //// LOGICS //////
    G4LogicalVolume* vessel_logic = new G4LogicalVolume(vessel_solid,
							MaterialsList::Steel316Ti(),
							"VESSEL");
    this->SetLogicalVolume(vessel_logic);

    G4Material* vessel_gas_mat =  MaterialsList::GXe(_pressure, _temperature);
    vessel_gas_mat->SetMaterialPropertiesTable(OpticalMaterialProperties::GXe(_pressure, _temperature, _sc_yield));
    G4LogicalVolume* vessel_gas_logic = new G4LogicalVolume(vessel_gas_solid, vessel_gas_mat,"VESSEL_GAS");
    _internal_logic_vol = vessel_gas_logic;
    new G4PVPlacement(0, G4ThreeVector(0.,0.,0.), vessel_gas_logic,
		      "VESSEL_GAS", vessel_logic, false, 0, false);


    /// SOURCE TUBES ////
    // Lateral
    G4Tubs* lateral_port_tube_solid = new G4Tubs("LATERAL_PORT", 0., (_port_tube_diam/2.+_port_tube_thickness),
						 (_lat_port_tube_length - _lat_nozzle_flange_high)/2., 0, twopi);
    G4LogicalVolume* lateral_port_tube_logic =
    new G4LogicalVolume(lateral_port_tube_solid, MaterialsList::Steel316Ti(), "LATERAL_PORT");

    //   G4ThreeVector pos_lateral_port(_lat_nozzle_x_pos + (_lat_port_tube_length - _lat_nozzle_flange_high)/2., 0., _lat_nozzle_z_pos);
    G4ThreeVector pos_lateral_port(_lat_nozzle_x_pos  + _lat_nozzle_high/2. - (_lat_port_tube_length - _lat_nozzle_flange_high)/2.,  0., _lat_nozzle_z_pos);
    new G4PVPlacement(G4Transform3D(*rot_lat, pos_lateral_port), lateral_port_tube_logic,
		      "LATERAL_PORT", vessel_gas_logic, false, 0, false);

    G4Tubs* lateral_port_tube_air_solid =
    new G4Tubs("LATERAL_PORT_AIR", 0., _port_tube_diam/2.,
	       (_lat_port_tube_length - _lat_nozzle_flange_high - _port_tube_window_thickn)/2.,
	       0, twopi);
     
  G4LogicalVolume* lateral_port_tube_air_logic =
    new G4LogicalVolume(lateral_port_tube_air_solid, 
			G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR"), "LATERAL_PORT_AIR");
  
  new G4PVPlacement(0,G4ThreeVector(0.,0.,-_port_tube_window_thickn/2.),
		    lateral_port_tube_air_logic, "LATERAL_PORT_AIR", 
		    lateral_port_tube_logic, false, 0, false);

  // The position of the source is assumed to be at the end of the tube, inside.
  // To be changed for different configurations
  _lateral_port_source_pos.setX(_lat_nozzle_x_pos  + _lat_nozzle_high/2. - (_lat_port_tube_length - _lat_nozzle_flange_high) + _port_tube_window_thickn);
  _lateral_port_source_pos.setY(0.);
  _lateral_port_source_pos.setZ(_lat_nozzle_z_pos);

   // Upper
    G4Tubs* upper_port_tube_solid = new G4Tubs("UPPER_PORT", 0., _port_tube_diam/2.+_port_tube_thickness,
						 (_up_port_tube_length - _up_nozzle_flange_high)/2., 0, twopi);
    G4LogicalVolume* upper_port_tube_logic =
    new G4LogicalVolume(upper_port_tube_solid, MaterialsList::Steel316Ti(), "UPPER_PORT");

    //    G4ThreeVector pos_upper_port(0., _up_nozzle_y_pos + (_up_port_tube_length - _up_nozzle_flange_high)/2., _up_nozzle_z_pos);
    G4ThreeVector pos_upper_port(0., _up_nozzle_y_pos + _up_nozzle_high/2. - (_up_port_tube_length - _up_nozzle_flange_high)/2., 0.);
    new G4PVPlacement(G4Transform3D(*rot_up, pos_upper_port), upper_port_tube_logic,
		      "UPPER_PORT", vessel_gas_logic, false, 0, false);

    G4Tubs* upper_port_tube_air_solid =
    new G4Tubs("UPPER_PORT_AIR", 0., _port_tube_diam/2.,
	       (_up_port_tube_length - _up_nozzle_flange_high - _port_tube_window_thickn)/2.,
	       0, twopi);
     
  G4LogicalVolume* upper_port_tube_air_logic =
    new G4LogicalVolume(upper_port_tube_air_solid, 
			G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR"), "UPPER_PORT_AIR");
  
  new G4PVPlacement(0,G4ThreeVector(0.,0.,-_port_tube_window_thickn/2.),
		    upper_port_tube_air_logic, "UPPER_PORT_AIR", 
		    upper_port_tube_logic, false, 0, false);

  // The position of the source is assumed to be at the end of the tube, inside.
  // To be changed for different configurations
  _upper_port_source_pos.setX(0.);
  _upper_port_source_pos.setY(_up_nozzle_y_pos  + _up_nozzle_high/2. - (_up_port_tube_length - _up_nozzle_flange_high) + _port_tube_window_thickn);
  _upper_port_source_pos.setZ(0.);
    
    // SETTING VISIBILITIES   //////////
    vessel_gas_logic->SetVisAttributes(G4VisAttributes::Invisible);
    if (_visibility) {
      G4VisAttributes titanium_col = nexus::TitaniumGrey();
      //   titanium_col.SetForceSolid(true);
      vessel_logic->SetVisAttributes(titanium_col);
      lateral_port_tube_logic->SetVisAttributes(titanium_col);
      upper_port_tube_logic->SetVisAttributes(titanium_col);
      G4VisAttributes air_col = nexus::Yellow();
      air_col.SetForceSolid(true);
      lateral_port_tube_air_logic->SetVisAttributes(air_col);
      upper_port_tube_air_logic->SetVisAttributes(air_col);
      vessel_logic->SetVisAttributes(titanium_col);
    } else {
      vessel_logic->SetVisAttributes(G4VisAttributes::Invisible);
    }


    //// VERTEX GENERATORS   //
    _body_gen   = new CylinderPointSampler(_vessel_in_diam/2., _vessel_tube_length, _vessel_thickness, 0.);
    _flange_gen = new CylinderPointSampler(vessel_out_diam/2., _flange_length,
    					    _flange_out_diam/2.-vessel_out_diam/2., 0., G4ThreeVector(0.,0.,0.));
    //trick to avoid vertex the vessel_gas-vessel interface -1*mm thickness
    _tracking_endcap_gen = new SpherePointSampler(_endcap_in_rad+1*mm, _endcap_thickness-1*mm, tracking_endcap_pos, 0,
    						  0., twopi, 0., _endcap_theta);
    _energy_endcap_gen = new SpherePointSampler(_endcap_in_rad+1*mm, _endcap_thickness-1*mm, energy_endcap_pos, 0,
    						0., twopi, 180.*deg - _endcap_theta, _endcap_theta);

   

    /// Calculating some prob
    G4double body_vol = vessel_tube_solid->GetCubicVolume() - vessel_gas_tube_solid->GetCubicVolume();
    G4double flange_vol = vessel_flange_solid->GetCubicVolume();
    G4double endcap_vol = vessel_tracking_endcap_solid->GetCubicVolume() - vessel_gas_tracking_endcap_solid->GetCubicVolume();
    G4double vol_tot = body_vol+ 2*flange_vol+ 2*endcap_vol;
    //    std::cout<<"VESSEL VOLUME: \t"<<vol_tot<<std::endl;
    _perc_tube_vol = body_vol/vol_tot; 
    _perc_endcap_vol = endcap_vol /vol_tot;
  }
   
  NextNewVessel::~NextNewVessel()
  {
    delete _body_gen;
    delete _flange_gen;     
    delete _tracking_endcap_gen;
    delete _energy_endcap_gen;
  }

  G4LogicalVolume* NextNewVessel::GetInternalLogicalVolume()
  {
    return _internal_logic_vol;
  }
  
  G4double NextNewVessel::GetUPNozzleZPosition()
  { 
    return _up_nozzle_z_pos;
  }

 G4double NextNewVessel::GetLATNozzleZPosition()
  { 
    return _lat_nozzle_z_pos;
  }

  G4ThreeVector NextNewVessel::GenerateVertex(const G4String& region) const
  {
    G4ThreeVector vertex(0., 0., 0.);
    
    // Vertex in the VESSEL volume
    if (region == "VESSEL") {
      G4double rand = G4UniformRand();
      if (rand < _perc_tube_vol) { //VESSEL_TUBE
	G4VPhysicalVolume *VertexVolume;
	do {
	  // std::cout<< "vessel tube \t"<< rand <<"\t"<< _perc_tube_vol << std::endl;      
	  vertex = _body_gen->GenerateVertex("BODY_VOL");   
	  VertexVolume = _geom_navigator->LocateGlobalPointAndSetup(vertex, 0, false);
	  // std::cout<<vertex<<std::endl;
	} while (VertexVolume->GetName() != "VESSEL");
      }
      //Vertex in ENDCAPCAPS
      else if (rand < (_perc_tube_vol+2*_perc_endcap_vol)){
	G4VPhysicalVolume *VertexVolume;
	do {
	  if (G4UniformRand() < 0.5){
	    //std::cout<< "tracking endcap "<< rand <<"\t"<< _perc_tube_vol+2*_perc_endcap_vol<< std::endl;
	    vertex = _tracking_endcap_gen->GenerateVertex("VOLUME");  // Tracking 
	  }
	  else {
	    //std::cout<< "energy endcap " << rand <<"\t"<< _perc_tube_vol+2*_perc_endcap_vol<< std::endl;
	    vertex = _energy_endcap_gen->GenerateVertex("VOLUME");  // Energy endcap	
	  }
	  VertexVolume = _geom_navigator->LocateGlobalPointAndSetup(vertex, 0, false);
	  //std::cout<<vertex<<std::endl;
	} while (VertexVolume->GetName() != "VESSEL");
      }    
      else { //FLANGES
   	if (G4UniformRand() < 0.5) {
	  vertex = _flange_gen->GenerateVertex("BODY_VOL");
	  vertex.setZ(vertex.z() + _flange_z_pos);
	  //std::cout<< "flange tracking \t"<<vertex.z() <<"\t"<< rand << std::endl;
    	}
    	else {
	  vertex = _flange_gen->GenerateVertex("BODY_VOL");
    	  vertex.setZ(vertex.z() - _flange_z_pos);
	  //std::cout<< "flange energy \t"<<vertex.z() <<"\t"<<rand<< std::endl;
   	}
      }
    }      
    else if (region =="SOURCE_PORT_ANODE") { 
      vertex = _lateral_port_source_pos; 
    }
    else if (region =="SOURCE_PORT_UP"){ 
      vertex = _upper_port_source_pos;
    }
    else if (region =="SOURCE_PORT_CATHODE"){ vertex = G4ThreeVector(_lat_nozzle_x_pos, 0.,-_lat_nozzle_z_pos);}
    else {
      G4Exception("[NextNewVessel]", "GenerateVertex()", FatalException,
		  "Unknown vertex generation region!");     
    }
    
    return vertex;
  }
  
}//end namespace nexus

