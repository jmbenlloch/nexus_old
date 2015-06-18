// ----------------------------------------------------------------------------
///  \file   Visibilities.h
///  \brief  General class to define colors for displaying geometries
///
///  \author  <paola.ferrario@ific.uv.es>
///  \date    17 Jun 2015
///  \version $Id$
//
///  Copyright (c) 2015 NEXT Collaboration. All rights reserved.
// ----------------------------------------------------------------------------
#include <G4VisAttributes.hh>

#ifndef VIS_H
#define VIS_H

namespace nexus {

  static G4VisAttributes Yellow()
  { G4Colour myColour; G4Colour::GetColour("yellow", myColour); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes White()
  { G4Colour myColour; G4Colour::GetColour("white", myColour); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes Red()
  { G4Colour myColour(1., 0., 0.); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes DarkGreen()
  { G4Colour myColour(0., .6, 0.); G4VisAttributes myAttr(myColour); return myAttr;}
   static G4VisAttributes LightGreen()
  { G4Colour myColour(.6,.9,.2); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes DirtyWhite()
  { G4Colour myColour(1., 1., .8); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes CopperBrown()
  { G4Colour myColour(.72, .45, .20); G4VisAttributes myAttr(myColour); return myAttr;}
   static G4VisAttributes Brown()
  { G4Colour myColour(.93, .87, .80); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes Blue()
  { G4Colour myColour(0., 0., 1.); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes LightBlue()
  { G4Colour myColour(.6, .8, .79); G4VisAttributes myAttr(myColour); return myAttr;}
  static G4VisAttributes Lilla()
  { G4Colour myColour(.5,.5,.7); G4VisAttributes myAttr(myColour); return myAttr;}
   static G4VisAttributes LightGrey()
  { G4Colour myColour(.7,.7,.7); G4VisAttributes myAttr(myColour); return myAttr;}

  
    
}  // end namespace nexus

#endif
