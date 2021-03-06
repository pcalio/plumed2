/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2016 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed.org for more information.

   This file is part of plumed, version 2.

   plumed is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   plumed is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with plumed.  If not, see <http://www.gnu.org/licenses/>.
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#include "core/ActionRegister.h"
#include "ContourFindingBase.h"
#include "tools/Random.h"

//+PLUMEDOC GRIDANALYSIS FIND_SPHERICAL_CONTOUR
/*
Find an isocontour in a three dimensional grid by searching over a Fibonacci sphere.

\par Examples

*/
//+ENDPLUMEDOC

namespace PLMD {
namespace gridtools {

class FindSphericalContour : public ContourFindingBase {
private:
  unsigned nbins;
  double min, max;
public:
  static void registerKeywords( Keywords& keys );
  explicit FindSphericalContour(const ActionOptions&ao);
  unsigned getNumberOfQuantities() const { return 2; }
  void compute( const unsigned& current, MultiValue& myvals ) const ;
};

PLUMED_REGISTER_ACTION(FindSphericalContour,"FIND_SPHERICAL_CONTOUR")

void FindSphericalContour::registerKeywords( Keywords& keys ){
  ContourFindingBase::registerKeywords( keys );
  keys.add("compulsory","NPOINTS","the number of points for which we are looking for the contour");
  keys.add("compulsory","INNER_RADIUS","the minimum radius on which to look for the contour");
  keys.add("compulsory","OUTER_RADIUS","the outer radius on which to look for the contour");
  keys.add("compulsory","NBINS","1","the number of discrete sections in which to divide the distance between the inner and outer radius when searching for a contour");
}

FindSphericalContour::FindSphericalContour(const ActionOptions&ao):
Action(ao),
ContourFindingBase(ao)
{
  if( ingrid->getDimension()!=3 ) error("input grid must be three dimensional");

  unsigned npoints; parse("NPOINTS",npoints);
  log.printf("  searching for %u points on dividing surface \n",npoints);
  parse("INNER_RADIUS",min); parse("OUTER_RADIUS",max); parse("NBINS",nbins);
  log.printf("  expecting to find dividing surface at radii between %f and %f \n",min,max);
  log.printf("  looking for contour in windows of length %f \n", (max-min)/nbins);
  // Set this here so the same set of grid points are used on every turn
  std::string vstring = "TYPE=fibonacci COMPONENTS=" + getLabel() + " COORDINATES=x,y,z PBC=F,F,F";
  createGrid( "grid", vstring ); mygrid->setNoDerivatives();
  setAveragingAction( mygrid, true ); mygrid->setupFibonacciGrid( npoints );

  checkRead();
  // Create a task list
  for(unsigned i=0;i<npoints;++i) addTaskToList( i );
  deactivateAllTasks();
  for(unsigned i=0;i<getFullNumberOfTasks();++i) taskFlags[i]=1;
  lockContributors();
}

void FindSphericalContour::compute( const unsigned& current, MultiValue& myvals ) const {
  // Generate contour point on inner sphere
  std::vector<double> contour_point(3), direction(3), der(3), tmp(3);
  // Retrieve this contour point from grid
  mygrid->getGridPointCoordinates( current, direction );
  // Now setup contour point on inner sphere
  for(unsigned j=0;j<3;++j){
     contour_point[j] = min*direction[j];
     direction[j] = (max-min)*direction[j] / static_cast<double>(nbins);
  }
  bool found=false;
  for(unsigned k=0;k<nbins;++k){
     for(unsigned j=0;j<3;++j) tmp[j] = contour_point[j] + direction[j];
     double val1 = getDifferenceFromContour( contour_point, der );
     double val2 = getDifferenceFromContour( tmp, der ); 
     if( val1*val2<0 ){
         findContour( direction, contour_point );
         double norm=0; for(unsigned j=0;j<3;++j) norm += contour_point[j]*contour_point[j];
         myvals.setValue( 1, sqrt(norm) ); found=true; break;
     }   
     for(unsigned j=0;j<3;++j) contour_point[j] = tmp[j]; 
  } 
  if( !found ) error("range does not bracket the dividing surface");
}

}
}
