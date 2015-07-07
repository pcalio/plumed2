/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   Copyright (c) 2011-2015 The plumed team
   (see the PEOPLE file at the root of the distribution for a list of names)

   See http://www.plumed-code.org for more information.

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
#include <vector>
#include <cmath>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cfloat>

#include "Grid.h"
#include "Tools.h"
#include "core/Value.h"
#include "File.h"
#include "Exception.h"
#include "KernelFunctions.h"

using namespace std;
namespace PLMD {

Grid::Grid(const std::string &funcl, std::vector<Value*> args, const vector<std::string> &gmin,
           const vector<std::string> &gmax, const vector<unsigned> &nbin, bool dospline, bool usederiv, bool doclear) {
// various checks
  plumed_massert(args.size() == gmin.size(), "grid dimensions in input do not match number of arguments");
  plumed_massert(args.size() == nbin.size(), "grid dimensions in input do not match number of arguments");
  plumed_massert(args.size() == gmax.size(), "grid dimensions in input do not match number of arguments");
  unsigned dim = gmax.size();
  std::vector<std::string> names;
  std::vector<bool> isperiodic;
  std::vector<string> pmin, pmax;
  names.resize(dim);
  isperiodic.resize(dim);
  pmin.resize(dim);
  pmax.resize(dim);
  for (unsigned int i = 0; i < dim; ++i) {
    names[i] = args[i]->getName();
    if (args[i]->isPeriodic()) {
      isperiodic[i] = true;
      args[i]->getDomain(pmin[i], pmax[i]);
    } else {
      isperiodic[i] = false;
      pmin[i] = "0.";
      pmax[i] = "0.";
    }
  }
// this is a value-independent initializator
  Init(funcl, names, gmin, gmax, nbin, dospline, usederiv, doclear, isperiodic, pmin, pmax);
}

Grid::Grid(const std::string &funcl, const std::vector<string> &names, const std::vector<std::string> &gmin,
           const vector<std::string> &gmax, const std::vector<unsigned> &nbin, bool dospline, bool usederiv, bool doclear, const std::vector<bool> &isperiodic, const std::vector<std::string> &pmin, const std::vector<std::string> &pmax) {
// this calls the initializator
  Init(funcl, names, gmin, gmax, nbin, dospline, usederiv, doclear, isperiodic, pmin, pmax);
}

void Grid::Init(const std::string &funcl, const std::vector<std::string> &names, const vector<std::string> &gmin,
                const std::vector<std::string> &gmax, const std::vector<unsigned> &nbin, bool dospline, bool usederiv, bool doclear,
                const std::vector<bool> &isperiodic, const std::vector<std::string> &pmin, const std::vector<std::string> &pmax) {
  fmt_ = "%14.9f";
// various checks
  plumed_massert(names.size() == gmin.size(), "grid dimensions in input do not match number of arguments");
  plumed_massert(names.size() == nbin.size(), "grid dimensions in input do not match number of arguments");
  plumed_massert(names.size() == gmax.size(), "grid dimensions in input do not match number of arguments");
  dimension_ = gmax.size();
  str_min_ = gmin;
  str_max_ = gmax;
  argnames.resize(dimension_);
  min_.resize(dimension_);
  max_.resize(dimension_);
  pbc_.resize(dimension_);
  indices_.resize(dimension_);
  for (unsigned int i = 0; i < dimension_; ++i) {
    argnames[i] = names[i];
    if (isperiodic[i]) {
      pbc_[i] = true;
      str_min_[i] = pmin[i];
      str_max_[i] = pmax[i];
    } else {
      pbc_[i] = false;
    }
    Tools::convert(str_min_[i], min_[i]);
    Tools::convert(str_max_[i], max_[i]);
    funcname = funcl;
    plumed_massert(max_[i] > min_[i], "maximum in grid must be larger than minimum");
    plumed_massert(nbin[i] > 0, "number of grid points must be greater than zero");
  }
  nbin_ = nbin;
  dospline_ = dospline;
  usederiv_ = usederiv;
  if (dospline_) {
    plumed_assert(dospline_ == usederiv_);
  }
  maxsize_ = 1;
  for (unsigned int i = 0; i < dimension_; ++i) {
    dx_.push_back((max_[i] - min_[i]) / static_cast<double>(nbin_[i]));
    if (!pbc_[i]) {
      max_[i] += dx_[i];
      nbin_[i] += 1;
    }
    maxsize_ *= nbin_[i];
  }
  if (doclear) {
    clear();
  }
}

void Grid::clear() {
  grid_.resize(maxsize_);
  if (usederiv_) {
    der_.resize(maxsize_);
  }
  for (unsigned long long i = 0; i < maxsize_; ++i) {
    grid_[i] = 0.0;
    if (usederiv_) {
      (der_[i]).resize(dimension_);
      for (unsigned int j = 0; j < dimension_; ++j) {
        der_[i][j] = 0.0;
      }
    }
  }
}

vector<std::string> Grid::getMin() const {
  return str_min_;
}

vector<std::string> Grid::getMax() const {
  return str_max_;
}

vector<double> Grid::getDx() const {
  return dx_;
}

double Grid::getBinVolume() const {
  double vol = 1.;
  for (unsigned i = 0; i < dx_.size(); ++i) {
    vol *= dx_[i];
  }
  return vol;
}

vector<bool> Grid::getIsPeriodic() const {
  return pbc_;
}

vector<unsigned> Grid::getNbin() const {
  return nbin_;
}

vector<string> Grid::getArgNames() const {
  return argnames;
}

unsigned long long Grid::getSize() const {
  return maxsize_;
}

unsigned long long Grid::getMaxSize() const {
  return maxsize_;
}

unsigned Grid::getDimension() const {
  return dimension_;
}

// we are flattening arrays using a column-major order
unsigned long long Grid::getIndex(const vector<unsigned> &indices) const {
  plumed_dbg_assert(indices.size() == dimension_);
  for (unsigned int i = 0; i < dimension_; i++)
    if (indices[i] >= nbin_[i]) {
      std::string is;
      Tools::convert(i, is);
      std::string msg = "ERROR: the system is looking for a value outside the grid along the " + is;
      plumed_merror(msg + " index!");
    }
  unsigned long long index = indices[dimension_ - 1];
  for (unsigned int i = dimension_ - 1; i > 0; --i) {
    index = index * nbin_[i - 1] + indices[i - 1];
  }
  return index;
}

unsigned long long Grid::getIndex(const vector<double> &x) const {
  plumed_dbg_assert(x.size() == dimension_);
  return getIndex(getIndices(x));
}

// we are flattening arrays using a column-major order
vector<unsigned> Grid::getIndices(unsigned long long index) const {
  vector<unsigned> indices(dimension_);
  unsigned kk = index;
  indices[0] = (index % nbin_[0]);
  for (unsigned int i = 1; i < dimension_ - 1; ++i) {
    kk = (kk - indices[i - 1]) / nbin_[i - 1];
    indices[i] = (kk % nbin_[i]);
  }
  if (dimension_ >= 2) {
    indices[dimension_ - 1] = ((kk - indices[dimension_ - 2]) / nbin_[dimension_ - 2]);
  }
  return indices;
}

vector<unsigned> Grid::getIndices(const vector<double> &x) const {
  plumed_dbg_assert(x.size() == dimension_);
  vector<unsigned> indices;
  for (unsigned int i = 0; i < dimension_; ++i) {
    indices.push_back(unsigned(floor((x[i] - min_[i]) / dx_[i])));
  }
  return indices;
}

vector<double> Grid::getPoint(const vector<unsigned> &indices) const {
  plumed_dbg_assert(indices.size() == dimension_);
  vector<double> x;
  for (unsigned int i = 0; i < dimension_; ++i) {
    x.push_back(min_[i] + (double)(indices[i])*dx_[i]);
  }
  return x;
}

vector<double> Grid::getPoint(unsigned long long index) const {
  plumed_dbg_assert(index < maxsize_);
  return getPoint(getIndices(index));
}

vector<double> Grid::getPoint(const vector<double> &x) const {
  plumed_dbg_assert(x.size() == dimension_);
  return getPoint(getIndices(x));
}

void Grid::getPoint(unsigned long long index, std::vector<double> &point) {
  plumed_dbg_assert(index < maxsize_);
  getIndices(index, indices_);
  getPoint(indices_, point);
}

void Grid::getPoint(const std::vector<unsigned> &indices, std::vector<double> &point) const {
  plumed_dbg_assert(indices.size() == dimension_);
  plumed_dbg_assert(point.size() == dimension_);
  for (unsigned int i = 0; i < dimension_; ++i) {
    point[i] = (min_[i] + (double)(indices[i]) * dx_[i]);
  }
}

void Grid::getPoint(const std::vector<double> &x, std::vector<double> &point) {
  plumed_dbg_assert(x.size() == dimension_);
  getIndices(x, indices_);
  getPoint(indices_, point);
}

// we are flattening arrays using a column-major order
void Grid::getIndices(unsigned long long index, vector<unsigned> &indices) {
  unsigned kk = index;
  indices[0] = (index % nbin_[0]);
  for (unsigned int i = 1; i < dimension_ - 1; ++i) {
    kk = (kk - indices[i - 1]) / nbin_[i - 1];
    indices[i] = (kk % nbin_[i]);
  }
  if (dimension_ >= 2) {
    indices[dimension_ - 1] = ((kk - indices[dimension_ - 2]) / nbin_[dimension_ - 2]);
  }
}

void Grid::getIndices(const vector<double> &x, vector<unsigned> &indices) {
  plumed_dbg_assert(x.size() == dimension_);
  for (unsigned int i = 0; i < dimension_; ++i) {
    indices.push_back(unsigned(floor((x[i] - min_[i]) / dx_[i])));
  }
}

vector<unsigned long long> Grid::getNeighbors
(const vector<unsigned> &indices, const vector<unsigned> &nneigh)const {
  plumed_dbg_assert(indices.size() == dimension_ && nneigh.size() == dimension_);
// Set up a small grid corresponding to all possible neighbors
// of the desired point if the grid were infinite in size.
  vector<unsigned long long> neighbors;
  vector<unsigned> small_bin(dimension_);
  unsigned small_nbin = 1;
  for (unsigned j = 0; j < dimension_; ++j) {
    small_bin[j] = (2 * nneigh[j] + 1);
    small_nbin *= small_bin[j];
  }
// For each point in the small neighbor grid, generate the indices for
// the point and decide whether it is a real neighbor or not.
  vector<unsigned> small_indices(dimension_);
  vector<unsigned> tmp_indices(dimension_);
  for (unsigned index = 0; index < small_nbin; ++index) {
    // First find the putative multidimensional index that this point would
    // correspond to in the greater grid neglecting boundaries.
    unsigned kk = index;
    small_indices[0] = (index % small_bin[0]);
    for (unsigned i = 1; i < dimension_ - 1; ++i) {
      kk = (kk - small_indices[i - 1]) / small_bin[i - 1];
      small_indices[i] = (kk % small_bin[i]);
    }
    if (dimension_ >= 2) {
      small_indices[dimension_ - 1] = ((kk - small_indices[dimension_ - 2]) / small_bin[dimension_ - 2]);
    }
    // Then check against the boundary conditions along each axis.
    unsigned ll = 0;
    for (unsigned i = 0; i < dimension_; ++i) {
      int i0 = small_indices[i] - nneigh[i] + indices[i];
      // If the point falls outside the grid and the boundaries
      // are cutoffs, then do not add this point as a neighbor.
      if (!pbc_[i] && i0 < 0) {
        continue;
      }
      if (!pbc_[i] && i0 >= nbin_[i]) {
        continue;
      }
      // If the point falls outside the grid and the boundaries
      // are periodic, then map the point into the grid and
      // assign the mapped point as a neighbor.
      if (pbc_[i] && i0 < 0) {
        i0 = nbin_[i] - (-i0) % nbin_[i];
      }
      if (pbc_[i] && i0 >= nbin_[i]) {
        i0 %= nbin_[i];
      }
      tmp_indices[ll] = ((unsigned)i0);
      ll++;
    }
    // Add the point to the list of neighbors if it actually fell in the grid.
    if (ll == dimension_) {
      neighbors.push_back(getIndex(tmp_indices));
    }
  }
  return neighbors;
}

vector<unsigned long long> Grid::getNeighbors
(const vector<double> &x, const vector<unsigned> &nneigh)const {
  plumed_dbg_assert(x.size() == dimension_ && nneigh.size() == dimension_);
  return getNeighbors(getIndices(x), nneigh);
}

vector<unsigned long long> Grid::getNeighbors
(unsigned long long index, const vector<unsigned> &nneigh)const {
  plumed_dbg_assert(index < maxsize_ && nneigh.size() == dimension_);
  return getNeighbors(getIndices(index), nneigh);
}

vector<unsigned long long> Grid::getSplineNeighbors(const vector<unsigned> &indices)const {
  plumed_dbg_assert(indices.size() == dimension_);
  vector<unsigned long long> neighbors;
  unsigned nneigh = unsigned(pow(2.0, int(dimension_)));
  for (unsigned int i = 0; i < nneigh; ++i) {
    unsigned tmp = i;
    vector<unsigned> nindices;
    for (unsigned int j = 0; j < dimension_; ++j) {
      unsigned i0 = tmp % 2 + indices[j];
      tmp /= 2;
      if (!pbc_[j] && i0 == nbin_[j]) {
        continue;
      }
      if (pbc_[j] && i0 == nbin_[j]) {
        i0 = 0;
      }
      nindices.push_back(i0);
    }
    if (nindices.size() == dimension_) {
      neighbors.push_back(getIndex(nindices));
    }
  }
  return neighbors;
}

vector<unsigned long long> Grid::getNearestNeighbors(const unsigned long long index) const {
  vector<unsigned long long> nearest_neighs = vector<unsigned long long>();
  for (unsigned i = 0; i < dimension_; i++) {
    vector<unsigned> neighsneeded = vector<unsigned>(dimension_, 0);
    neighsneeded[i] = 1;
    vector<unsigned long long> singledim_nearest_neighs = getNeighbors(index, neighsneeded);
    for (unsigned j = 0; j < singledim_nearest_neighs.size(); j++) {
      unsigned long long neigh = singledim_nearest_neighs[j];
      if (neigh != index) {
        nearest_neighs.push_back(neigh);
      }
    }
  }
  return nearest_neighs;
}

vector<unsigned long long> Grid::getNearestNeighbors(const vector<unsigned> &indices) const {
  plumed_dbg_assert(indices.size() == dimension_);
  return getNearestNeighbors(getIndex(indices));
}

void Grid::addKernel(const KernelFunctions &kernel) {
  plumed_dbg_assert(kernel.ndim() == dimension_);
  std::vector<unsigned> nneighb = kernel.getSupport(dx_);
  std::vector<unsigned long long> neighbors = getNeighbors(kernel.getCenter(), nneighb);
  std::vector<double> xx(dimension_);
  std::vector<Value*> vv(dimension_);
  std::string str_min, str_max;
  for (unsigned i = 0; i < dimension_; ++i) {
    vv[i] = new Value();
    if (pbc_[i]) {
      Tools::convert(min_[i], str_min);
      Tools::convert(max_[i], str_max);
      vv[i]->setDomain(str_min, str_max);
    } else {
      vv[i]->setNotPeriodic();
    }
  }
  double newval;
  std::vector<double> der(dimension_);
  for (unsigned i = 0; i < neighbors.size(); ++i) {
    unsigned long long ineigh = neighbors[i];
    getPoint(ineigh, xx);
    for (unsigned j = 0; j < dimension_; ++j) {
      vv[j]->set(xx[j]);
    }
    newval = kernel.evaluate(vv, der, usederiv_);
    if (usederiv_) {
      addValueAndDerivatives(ineigh, newval, der);
    } else {
      addValue(ineigh, newval);
    }
  }
  for (unsigned i = 0; i < dimension_; ++i) {
    delete vv[i];
  }
}

double Grid::getValue(unsigned long long index) const {
  plumed_dbg_assert(index < maxsize_);
  return grid_[index];
}

double Grid::getMinValue() const {
  double minval;
  minval = DBL_MAX;
  for (unsigned long long i = 0; i < grid_.size(); ++i) {
    if (grid_[i] < minval) {
      minval = grid_[i];
    }
  }
  return minval;
}

double Grid::getMaxValue() const {
  double maxval;
  maxval = DBL_MIN;
  for (unsigned long long i = 0; i < grid_.size(); ++i) {
    if (grid_[i] > maxval) {
      maxval = grid_[i];
    }
  }
  return maxval;
}


double Grid::getValue(const vector<unsigned> &indices) const {
  return getValue(getIndex(indices));
}

double Grid::getValue(const vector<double> &x) const {
  if (!dospline_) {
    return getValue(getIndex(x));
  } else {
    vector<double> der(dimension_);
    return getValueAndDerivatives(x, der);
  }
}

double Grid::getValueAndDerivatives
(unsigned long long index, vector<double> &der) const {
  plumed_dbg_assert(index < maxsize_ && usederiv_ && der.size() == dimension_);
  der = der_[index];
  return grid_[index];
}

double Grid::getValueAndDerivatives
(const vector<unsigned> &indices, vector<double> &der) const {
  return getValueAndDerivatives(getIndex(indices), der);
}

double Grid::getValueAndDerivatives
(const vector<double> &x, vector<double> &der) const {
  plumed_dbg_assert(der.size() == dimension_ && usederiv_);
  if (dospline_) {
    double X, X2, X3, value;
    vector<double> fd(dimension_);
    vector<double> C(dimension_);
    vector<double> D(dimension_);
    vector<double> dder(dimension_);
// reset
    value = 0.0;
    for (unsigned int i = 0; i < dimension_; ++i) {
      der[i] = 0.0;
    }
    vector<unsigned> indices = getIndices(x);
    vector<unsigned long long> neigh = getSplineNeighbors(indices);
    vector<double>   xfloor = getPoint(x);

// loop over neighbors
    for (unsigned int ipoint = 0; ipoint < neigh.size(); ++ipoint) {
      double grid = getValueAndDerivatives(neigh[ipoint], dder);
      vector<unsigned> nindices = getIndices(neigh[ipoint]);
      double ff = 1.0;
      for (unsigned j = 0; j < dimension_; ++j) {
        int x0 = 1;
        if (nindices[j] == indices[j]) {
          x0 = 0;
        }
        double dx = getDx()[j];
        X = fabs((x[j] - xfloor[j]) / dx - (double)x0);
        X2 = X * X;
        X3 = X2 * X;
        double yy;
        if (fabs(grid) < 0.0000001) {
          yy = 0.0;
        } else {
          yy = -dder[j] / grid;
        }
        C[j] = (1.0 - 3.0 * X2 + 2.0 * X3) - (x0 ? -1.0 : 1.0) * yy * (X - 2.0 * X2 + X3) * dx;
        D[j] = (-6.0 * X + 6.0 * X2) - (x0 ? -1.0 : 1.0) * yy * (1.0 - 4.0 * X + 3.0 * X2) * dx;
        D[j] *= (x0 ? -1.0 : 1.0) / dx;
        ff *= C[j];
      }
      for (unsigned j = 0; j < dimension_; ++j) {
        fd[j] = D[j];
        for (unsigned i = 0; i < dimension_; ++i) if (i != j) {
            fd[j] *= C[i];
          }
      }
      value += grid * ff;
      for (unsigned j = 0; j < dimension_; ++j) {
        der[j] += grid * fd[j];
      }
    }
    return value;
  } else {
    return getValueAndDerivatives(getIndex(x), der);
  }
}

void Grid::setValue(unsigned long long index, double value) {
  plumed_dbg_assert(index < maxsize_ && !usederiv_);
  grid_[index] = value;
}

void Grid::setValue(const vector<unsigned> &indices, double value) {
  setValue(getIndex(indices), value);
}

void Grid::setValueAndDerivatives
(unsigned long long index, double value, vector<double> &der) {
  plumed_dbg_assert(index < maxsize_ && usederiv_ && der.size() == dimension_);
  grid_[index] = value;
  der_[index] = der;
}

void Grid::setValueAndDerivatives
(const vector<unsigned> &indices, double value, vector<double> &der) {
  setValueAndDerivatives(getIndex(indices), value, der);
}

void Grid::addValue(unsigned long long index, double value) {
  plumed_dbg_assert(index < maxsize_ && !usederiv_);
  grid_[index] += value;
}

void Grid::addValue(const vector<unsigned> &indices, double value) {
  addValue(getIndex(indices), value);
}

void Grid::addValueAndDerivatives
(unsigned long long index, double value, vector<double> &der) {
  plumed_dbg_assert(index < maxsize_ && usederiv_ && der.size() == dimension_);
  grid_[index] += value;
  for (unsigned int i = 0; i < dimension_; ++i) {
    der_[index][i] += der[i];
  }
}

void Grid::addValueAndDerivatives
(const vector<unsigned> &indices, double value, vector<double> &der) {
  addValueAndDerivatives(getIndex(indices), value, der);
}

void Grid::scaleAllValuesAndDerivatives(const double &scalef) {
  if (usederiv_) {
    for (unsigned long long i = 0; i < grid_.size(); ++i) {
      grid_[i] *= scalef;
      for (unsigned j = 0; j < dimension_; ++j) {
        der_[i][j] *= scalef;
      }
    }
  } else {
    for (unsigned long long i = 0; i < grid_.size(); ++i) {
      grid_[i] *= scalef;
    }
  }
}

void Grid::logAllValuesAndDerivatives(const double &scalef) {
  if (usederiv_) {
    for (unsigned long long i = 0; i < grid_.size(); ++i) {
      grid_[i] = scalef * log(grid_[i]);
      for (unsigned j = 0; j < dimension_; ++j) {
        der_[i][j] = scalef / der_[i][j];
      }
    }
  } else {
    for (unsigned long long i = 0; i < grid_.size(); ++i) {
      grid_[i] = scalef * log(grid_[i]);
    }
  }
}

void Grid::setMinToZero() {
  double min = grid_[0];
  for (unsigned long long i = 1; i < grid_.size(); ++i) if (grid_[i] < min) {
      min = grid_[i];
    }
  for (unsigned long long i = 0; i < grid_.size(); ++i) {
    grid_[i] -= min;
  }
}

void Grid::applyFunctionAllValuesAndDerivatives(double(*func)(double val), double(*funcder)(double valder)) {
  if (usederiv_) {
    for (unsigned long long i = 0; i < grid_.size(); ++i) {
      grid_[i] = func(grid_[i]);
      for (unsigned j = 0; j < dimension_; ++j) {
        der_[i][j] = funcder(der_[i][j]);
      }
    }
  } else {
    for (unsigned long long i = 0; i < grid_.size(); ++i) {
      grid_[i] = func(grid_[i]);
    }
  }
}

bool indexed_lt(pair<unsigned long long, double> const &x, pair<unsigned long long, double> const   &y) {
  return x.second < y.second;
}

double Grid::findMaximalPathMinimum(const std::vector<double> &source, const std::vector<double> &sink) {
  plumed_dbg_assert(source.size() == dimension_);
  plumed_dbg_assert(sink.size() == dimension_);
  // Start and end indices
  unsigned long long source_idx = getIndex(source);
  unsigned long long sink_idx = getIndex(sink);
  // Path cost
  double maximal_minimum;
  // In one dimension, path searching is very easy--either go one way if it's not periodic,
  // or go both ways if it is periodic. There's no reason to pay the cost of Dijkstra.
  if (dimension_ == 1) {
    // Do a search from the grid source to grid sink that does not
    // cross the grid boundary.
    double curr_min_bias = getValue(source_idx);
    // Either search from a high source to a low sink.
    if (source_idx > sink_idx) {
      for (unsigned long long i = source_idx; i >= sink_idx; i--) {
        if (curr_min_bias == 0.0) {
          break;
        }
        curr_min_bias = fmin(curr_min_bias, getValue(i));
      }
      // Or search from a low source to a high sink.
    } else if (source_idx < sink_idx) {
      for (unsigned long long i = source_idx; i <= sink_idx; i++) {
        if (curr_min_bias == 0.0) {
          break;
        }
        curr_min_bias = fmin(curr_min_bias, getValue(i));
      }
    }
    maximal_minimum = curr_min_bias;
    // If the grid is periodic, also do the search that crosses
    // the grid boundary.
    if (pbc_[0]) {
      double curr_min_bias = getValue(source_idx);
      // Either go from a high source to the upper boundary and
      // then from the bottom boundary to the sink
      if (source_idx > sink_idx) {
        for (unsigned long long i = source_idx; i < maxsize_; i++) {
          if (curr_min_bias == 0.0) {
            break;
          }
          curr_min_bias = fmin(curr_min_bias, getValue(i));
        }
        for (unsigned long long i = 0; i <= sink_idx; i++) {
          if (curr_min_bias == 0.0) {
            break;
          }
          curr_min_bias = fmin(curr_min_bias, getValue(i));
        }
        // Or go from a low source to the bottom boundary and
        // then from the high boundary to the sink
      } else if (source_idx < sink_idx) {
        for (unsigned long long i = source_idx; i > 0; i--) {
          if (curr_min_bias == 0.0) {
            break;
          }
          curr_min_bias = fmin(curr_min_bias, getValue(i));
        }
        curr_min_bias = fmin(curr_min_bias, getValue(0));
        for (unsigned long long i = maxsize_ - 1; i <= sink_idx; i--) {
          if (curr_min_bias == 0.0) {
            break;
          }
          curr_min_bias = fmin(curr_min_bias, getValue(i));
        }
      }
      // If the boundary crossing paths was more biased, it's
      // minimal bias replaces the non-boundary-crossing path's
      // minimum.
      maximal_minimum = fmax(maximal_minimum, curr_min_bias);
    }
    // The one dimensional path search is complete.
    return maximal_minimum;
  // In two or more dimensions, path searching isn't trivial and we really
  // do need to use a path search algorithm. Dijkstra is the simplest decent
  // one. Using it we've never found the path search to be performance
  // limiting in any solvated biomolecule test system, but faster options are
  // easy to imagine if they become necessary. NB-In this case, we're actually 
  // using a greedy variant of Dijkstra's algorithm where the first possible 
  // path to a point always controls the path cost to that point. The structure
  // of the cost function in this case guarantees that the calculated costs will 
  // be correct using this variant even though fine details of the paths may not
  // match a normal Dijkstra search.
  } else if (dimension_ > 1) {
    // Prepare calculation temporaries for Dijkstra's algorithm.
    // Minimal path costs from source to a given grid point
    vector<double> mins_from_source = vector<double>(maxsize_, -1.0);
    // Heap for tracking available steps, steps are recorded as std::pairs of
    // an index and a value.
    vector< pair<unsigned long long, double> > next_steps;
    pair<unsigned long long, double> curr_indexed_val;
    make_heap(next_steps.begin(), next_steps.end(), indexed_lt);
    // The search begins at the source index.
    next_steps.push_back(pair<unsigned long long, double>(source_idx, getValue(source_idx)));
    push_heap(next_steps.begin(), next_steps.end(), indexed_lt);
    // At first no points have been examined and the optimal path has not been found.
    unsigned long long n_examined = 0;
    bool path_not_found = true;
    // Until a path is found,
    while (path_not_found) {
      // Examine the grid point currently most accessible from
      // the set of all previously explored grid points by popping
      // it from the top of the heap.
      pop_heap(next_steps.begin(), next_steps.end(), indexed_lt);
      curr_indexed_val = next_steps.back();
      next_steps.pop_back();
      n_examined++;
      // Check if this point is the sink point, and if so
      // finish the loop.
      if (curr_indexed_val.first == sink_idx) {
        path_not_found = false;
        maximal_minimum = curr_indexed_val.second;
        break;
        // Check if this point has reached the worst possible
        // value, and if so stop looking for paths.
      } else if (curr_indexed_val.second == 0.0) {
        maximal_minimum = 0.0;
        break;
      }
      // If the search is not over, add this grid point's neighbors to the
      // possible next points to search for the sink.
      vector<unsigned long long> neighs = getNearestNeighbors(curr_indexed_val.first);
      for (unsigned k = 0; k < neighs.size(); k++) {
	      unsigned long long i = neighs[k];
        // If the neighbor has not already been added to the list of possible next steps,
        if (mins_from_source[i] == -1.0) {
          // Set the cost to reach it via a path through the current point being examined.
          mins_from_source[i] = fmin(curr_indexed_val.second, getValue(i));
          // Add the neighboring point to the heap of potential next steps.
          next_steps.push_back(pair<unsigned long long, double>(i, mins_from_source[i]));
          push_heap(next_steps.begin(), next_steps.end(), indexed_lt);
        }
      }
      // Move on to the next best looking step along any of the paths
      // growing from the source.
    }
    // The multidimensional path search is now complete.
    return maximal_minimum;
  }
}

void Grid::setDerivFromValues(const unsigned long long index) {
  // Along each dimension, take a centered finite difference
  // for the derivative. If one is against a hard boundary,
  // set the derivative to zero as if the boundaries are reflective.
  for (unsigned i = 0; i < dimension_; i++) {
    vector<unsigned> neighsneeded = vector<unsigned>(dimension_, 0);
    neighsneeded[i] = 1;
    // These GetNeighbors calls return up to 3 neighbors in a defined ordering
    // matching the grid's unwrapped-boundaries-ordering. The point of interest
    // is considered one of its own neighbors.
    vector<unsigned long long> neighs = getNeighbors(index, neighsneeded);
    // If there are three points, use the automatic ordering.
    if (neighs.size() == 3) {
      der_[index][i] = (grid_[neighs[2]] - grid_[neighs[0]]) / (2 * dx_[i]);
      // If there are only two, a hard boundary is nearby. Set to zero.
    } else {
      der_[index][i] = 0.0;
    }
  }
}

void Grid::setDerivFromValues(const vector<unsigned> &indices) {
  setDerivFromValues(getIndex(indices));
}


void Grid::writeHeader(OFile &ofile) {
  for (unsigned i = 0; i < dimension_; ++i) {
    ofile.addConstantField("min_" + argnames[i]);
    ofile.addConstantField("max_" + argnames[i]);
    ofile.addConstantField("nbins_" + argnames[i]);
    ofile.addConstantField("periodic_" + argnames[i]);
  }
}

void Grid::writeToFile(OFile &ofile) {
  vector<double> xx(dimension_);
  vector<double> der(dimension_);
  double f;
  writeHeader(ofile);
  for (unsigned long long i = 0; i < getSize(); ++i) {
    xx = getPoint(i);
    if (usederiv_) {
      f = getValueAndDerivatives(i, der);
    } else {
      f = getValue(i);
    }
    if (i > 0 && dimension_ > 1 && getIndices(i)[dimension_ - 2] == 0) {
      ofile.printf("\n");
    }
    for (unsigned j = 0; j < dimension_; ++j) {
      ofile.printField("min_" + argnames[j], str_min_[j]);
      ofile.printField("max_" + argnames[j], str_max_[j]);
      ofile.printField("nbins_" + argnames[j], static_cast<int>(nbin_[j]));
      if (pbc_[j]) {
        ofile.printField("periodic_" + argnames[j], "true");
      } else {
        ofile.printField("periodic_" + argnames[j], "false");
      }
    }
    for (unsigned j = 0; j < dimension_; ++j) {
      ofile.fmtField(" " + fmt_);
      ofile.printField(argnames[j], xx[j]);
    }
    ofile.fmtField(" " + fmt_);
    ofile.printField(funcname, f);
    if (usederiv_) for (unsigned j = 0; j < dimension_; ++j) {
        ofile.fmtField(" " + fmt_);
        ofile.printField("der_" + argnames[j] , der[j]);
      }
    ofile.printField();
  }
}

void Grid::writeCubeFile(OFile& ofile){
  plumed_assert( dimension_==3 );
  ofile.printf("PLUMED CUBE FILE\n");
  ofile.printf("OUTER LOOP: X, MIDDLE LOOP: Y, INNER LOOP: Z\n");
  ofile.printf("%d %f %f %f\n",0,0.0,0.0,0.0); // Number of atoms followed by position of origin
  ofile.printf("%d %f %f %f\n",nbin_[0],dx_[0],0.0,0.0);  // Number of bins in each direction followed by 
  ofile.printf("%d %f %f %f\n",nbin_[1],0.0,dx_[1],0.0);  // shape of voxel
  ofile.printf("%d %f %f %f\n",nbin_[2],0.0,0.0,dx_[2]);
  std::vector<unsigned> pp(3);
  for(pp[0]=0;pp[0]<nbin_[0];++pp[0]){
      for(pp[1]=0;pp[1]<nbin_[1];++pp[1]){
          for(pp[2]=0;pp[2]<nbin_[2];++pp[2]){
              ofile.printf("%f ",getValue(pp) );
              if(pp[2]%6==5) ofile.printf("\n");
          }
          ofile.printf("\n");
     }
  }
}

Grid* Grid::create(const std::string &funcl, std::vector<Value*> args, IFile &ifile,
                   const vector<std::string> &gmin, const vector<std::string> &gmax,
                   const vector<unsigned> &nbin, bool dosparse, bool dospline, bool doder) {
  double tmp_check, tmp_ref;
  
  Grid* grid = Grid::create(funcl, args, ifile, dosparse, dospline, doder);
  
  std::vector<unsigned> cbin(grid->getNbin());
  std::vector<std::string> cmin(grid->getMin()), cmax(grid->getMax());
  for (unsigned i = 0; i < args.size(); ++i) {
    Tools::convert(cmin[i], tmp_check);
    Tools::convert(gmin[i], tmp_ref);
    plumed_massert(Tools::compare_float(tmp_ref, tmp_check), "mismatched grid min");
    Tools::convert(cmax[i], tmp_check);
    Tools::convert(gmax[i], tmp_ref);
    plumed_massert(Tools::compare_float(tmp_ref, tmp_check), "mismatched grid max");
    if (args[i]->isPeriodic()) {
      plumed_massert(cbin[i] == nbin[i], "mismatched grid nbins");
    } else {
      plumed_massert((cbin[i] - 1) == nbin[i], "mismatched grid nbins");
    }
  }
  return grid;
}

Grid* Grid::create(const std::string &funcl, std::vector<Value*> args, IFile &ifile, bool dosparse, bool dospline, bool doder) {
  Grid* grid = NULL;
  double tmp_check, tmp_ref;
  unsigned nvar = args.size();
  bool hasder = false;
  std::string pstring;
  std::vector<int> gbin1(nvar);
  std::vector<unsigned> gbin(nvar);
  std::vector<std::string> labels(nvar), gmin(nvar), gmax(nvar);
  std::vector<std::string> fieldnames;
  ifile.scanFieldList(fieldnames);
// Retrieve names for fields
  for (unsigned i = 0; i < args.size(); ++i) {
    labels[i] = args[i]->getName();
  }
// And read the stuff from the header
  plumed_massert(ifile.FieldExist(funcl) , "no column labelled " + funcl + " in grid input");
  for (unsigned i = 0; i < args.size(); ++i) {
    ifile.scanField("min_" + labels[i], gmin[i]);
    ifile.scanField("max_" + labels[i], gmax[i]);
    ifile.scanField("periodic_" + labels[i], pstring);
    ifile.scanField("nbins_" + labels[i], gbin1[i]);
    plumed_assert(gbin1[i] > 0);
    if (args[i]->isPeriodic()) {
      plumed_massert(pstring == "true", "input value is periodic but grid is not");
      std::string pmin, pmax;
      args[i]->getDomain(pmin, pmax);
      gbin[i] = gbin1[i];
      Tools::convert(pmin, tmp_check);
      Tools::convert(gmin[i], tmp_ref);
      plumed_massert(Tools::compare_float(tmp_ref, tmp_check), "mismatched between grid boundary min and period min");
      Tools::convert(pmax, tmp_check);
      Tools::convert(gmax[i], tmp_ref);
      plumed_massert(Tools::compare_float(tmp_ref, tmp_check), "mismatched between grid boundary max and period max");
    } else {
      gbin[i] = gbin1[i] - 1; // Note header in grid file indicates one more bin that there should be when data is not periodic
      plumed_massert(pstring == "false", "input value is not periodic but grid is");
    }
    hasder = ifile.FieldExist("der_" + args[i]->getName());
    if (doder && !hasder) {
      plumed_merror("missing derivatives from grid file");
    }
    for (unsigned j = 0; j < fieldnames.size(); ++j) {
      for (unsigned k = i + 1; k < args.size(); ++k) {
        if (fieldnames[j] == labels[k]) {
          plumed_merror("arguments in input are not in same order as in grid file");
        }
      }
      if (fieldnames[j] == labels[i]) {
        break;
      }
    }
  }
  if (!dosparse) {
    grid = new Grid(funcl, args, gmin, gmax, gbin, dospline, doder);
  } else {
    grid = new SparseGrid(funcl, args, gmin, gmax, gbin, dospline, doder);
  }
  vector<double> xx(nvar), dder(nvar);
  vector<double> dx = grid->getDx();
  double f, x;
  while (ifile.scanField(funcl, f)) {
    for (unsigned i = 0; i < nvar; ++i) {
      ifile.scanField(labels[i], x);
      xx[i] = x + dx[i] / 2.0;
      ifile.scanField("min_" + labels[i], gmin[i]);
      ifile.scanField("max_" + labels[i], gmax[i]);
      ifile.scanField("nbins_" + labels[i], gbin1[i]);
      ifile.scanField("periodic_" + labels[i], pstring);
    }
    if (hasder) {
      for (unsigned i = 0; i < nvar; ++i) {
        ifile.scanField("der_" + args[i]->getName(), dder[i]);
      }
    }
    unsigned index = grid->getIndex(xx);
    if (doder) {
      grid->setValueAndDerivatives(index, f, dder);
    } else {
      grid->setValue(index, f);
    }
    ifile.scanField();
  }
  return grid;
}

// Sparse version of grid with map
void SparseGrid::clear() {
  map_.clear();
}

unsigned long long SparseGrid::getSize() const {
  return map_.size();
}

double SparseGrid::getValue(unsigned long long index)const {
  plumed_assert(index < maxsize_);
  double value = 0.0;
  iterator it = map_.find(index);
  if (it != map_.end()) {
    value = it->second;
  }
  return value;
}

double SparseGrid::getValueAndDerivatives
(unsigned long long index, vector<double> &der)const {
  plumed_assert(index < maxsize_ && usederiv_ && der.size() == dimension_);
  double value = 0.0;
  for (unsigned int i = 0; i < dimension_; ++i) {
    der[i] = 0.0;
  }
  iterator it = map_.find(index);
  if (it != map_.end()) {
    value = it->second;
  }
  iterator_der itder = der_.find(index);
  if (itder != der_.end()) {
    der = itder->second;
  }
  return value;
}

void SparseGrid::setValue(unsigned long long index, double value) {
  plumed_assert(index < maxsize_ && !usederiv_);
  map_[index] = value;
}

void SparseGrid::setValueAndDerivatives
(unsigned long long index, double value, vector<double> &der) {
  plumed_assert(index < maxsize_ && usederiv_ && der.size() == dimension_);
  map_[index] = value;
  der_[index] = der;
}

void SparseGrid::addValue(unsigned long long index, double value) {
  plumed_assert(index < maxsize_ && !usederiv_);
  map_[index] += value;
}

void SparseGrid::addValueAndDerivatives
(unsigned long long index, double value, vector<double> &der) {
  plumed_assert(index < maxsize_ && usederiv_ && der.size() == dimension_);
  map_[index] += value;
  der_[index].resize(dimension_);
  for (unsigned int i = 0; i < dimension_; ++i) {
    der_[index][i] += der[i];
  }
}

void SparseGrid::writeToFile(OFile &ofile) {
  vector<double> xx(dimension_);
  vector<double> der(dimension_);
  double f;
  writeHeader(ofile);
  ofile.fmtField(" " + fmt_);
  for (iterator it = map_.begin(); it != map_.end(); ++it) {
    unsigned long long i = (*it).first;
    xx = getPoint(i);
    if (usederiv_) {
      f = getValueAndDerivatives(i, der);
    } else {
      f = getValue(i);
    }
    if (i > 0 && dimension_ > 1 && getIndices(i)[dimension_ - 2] == 0) {
      ofile.printf("\n");
    }
    for (unsigned j = 0; j < dimension_; ++j) {
      ofile.printField("min_" + argnames[j], str_min_[j]);
      ofile.printField("max_" + argnames[j], str_max_[j]);
      ofile.printField("nbins_" + argnames[j], static_cast<int>(nbin_[j]));
      if (pbc_[j]) {
        ofile.printField("periodic_" + argnames[j], "true");
      } else {
        ofile.printField("periodic_" + argnames[j], "false");
      }
    }
    for (unsigned j = 0; j < dimension_; ++j) {
      ofile.printField(argnames[j], xx[j]);
    }
    ofile.printField(funcname, f);
    if (usederiv_) {
      for (unsigned j = 0; j < dimension_; ++j) {
        ofile.printField("der_" + argnames[j], der[j]);
      }
    }
    ofile.printField();
  }
}

void Grid::projectOnLowDimension(double &val, std::vector<int> &vHigh, WeightBase * ptr2obj ){
    unsigned i=0;
    for(i=0;i<vHigh.size();i++){
       if(vHigh[i]<0){// this bin needs to be integrated out 
          // parallelize here???
    	  for(unsigned j=0;j<(getNbin())[i];j++){
            vHigh[i]=int(j);  
            projectOnLowDimension(val,vHigh,ptr2obj); // recursive function: this is the core of the mechanism
            vHigh[i]=-1;
          } 
          return; // 
       }
    }
    // when there are no more bin to dig in then retrieve the value 
    if(i==vHigh.size()){
        //std::cerr<<"POINT: "; 
        //for(unsigned j=0;j<vHigh.size();j++){
        //   std::cerr<<vHigh[j]<<" ";
        //} 
        std::vector<unsigned> vv(vHigh.size()); 
        for(unsigned j=0;j<vHigh.size();j++)vv[j]=unsigned(vHigh[j]);
        //
        // this is the real assignment !!!!! (hack this to have bias or other stuff)
        //

        // this case: produce fes
        //val+=exp(beta*getValue(vv)) ;
        double myv=getValue(vv);
        val=ptr2obj->projectInnerLoop(val,myv) ;
        // to be added: bias (same as before without negative sign) 
        //std::cerr<<" VAL: "<<val <<endl;
    }
}

Grid Grid::project(const std::vector<std::string> & proj , WeightBase *ptr2obj ){
         // find extrema only for the projection
         vector<string>   smallMin,smallMax;
         vector<unsigned> smallBin;
         vector<unsigned> dimMapping;
         vector<bool> smallIsPeriodic;
         vector<string> smallName;

         // check if the two key methods are there
         WeightBase* pp = dynamic_cast<WeightBase*>(ptr2obj);
         if (!pp)plumed_merror("This WeightBase is not complete: you need a projectInnerLoop and projectOuterLoop ");
 
         for(unsigned j=0;j<proj.size();j++){
              for(unsigned i=0;i<getArgNames().size();i++){
                    if(proj[j]==getArgNames()[i]){ 
	    	         unsigned offset;		 
 	    	         // note that at sizetime the non periodic dimension get a bin more  	
                         if(getIsPeriodic()[i]){offset=0;}else{offset=1;}
                         smallMax.push_back(getMax()[i]);
                         smallMin.push_back(getMin()[i]);
                         smallBin.push_back(getNbin()[i]-offset);
			 smallIsPeriodic.push_back(getIsPeriodic()[i]);
                         dimMapping.push_back(i);
			 smallName.push_back(getArgNames()[i]);
                         break;
                    }
              }
         }
         Grid smallgrid("projection",smallName,smallMin,smallMax,smallBin,false,false,true,smallIsPeriodic,smallMin,smallMax);  
         // check that the two grids are commensurate 
         for(unsigned i=0;i<dimMapping.size();i++){
              plumed_massert(  (smallgrid.getMax())[i] == (getMax())[dimMapping[i]],  "the two input grids are not compatible in max"   );  
              plumed_massert(  (smallgrid.getMin())[i] == (getMin())[dimMapping[i]],  "the two input grids are not compatible in min"   );  
              plumed_massert(  (smallgrid.getNbin())[i]== (getNbin())[dimMapping[i]], "the two input grids are not compatible in bin"   );  
         }
         vector<unsigned> toBeIntegrated;
         for(unsigned i=0;i<getArgNames().size();i++){
              bool doappend=true;
         	for(unsigned j=0;j<dimMapping.size();j++){
                 if(dimMapping[j]==i){doappend=false; break;}  
              }
              if(doappend)toBeIntegrated.push_back(i);
         }
         //for(unsigned i=0;i<dimMapping.size();i++ ){
         //     cerr<<"Dimension to preserve "<<dimMapping[i]<<endl;
         //}
         //for(unsigned i=0;i<toBeIntegrated.size();i++ ){
         //     cerr<<"Dimension to integrate "<<toBeIntegrated[i]<<endl;
         //}

         // loop over all the points in the Grid, find the corresponding fixed index, rotate over all the other ones  
         for(unsigned i=0;i<smallgrid.getSize();i++){
                 std::vector<unsigned> v;
                 v=smallgrid.getIndices(i);
                 std::vector<int> vHigh((getArgNames()).size(),-1);   
                 for(unsigned j=0;j<dimMapping.size();j++)vHigh[dimMapping[j]]=int(v[j]);
                 // the vector vhigh now contains at the beginning the index of the low dimension and -1 for the values that need to be integrated 
                 double initval=0.;  
                 projectOnLowDimension(initval,vHigh, ptr2obj); 
                 smallgrid.setValue(i,initval);  
         }
         // reset to zero just for biasing (this option can be evtl enabled in a future...) 
         //double vmin;vmin=-smallgrid.getMinValue()+1;
         for(unsigned i=0;i<smallgrid.getSize();i++){
         //         //if(dynamic_cast<BiasWeight*>(ptr2obj)){
	 //         //        smallgrid.addValue(i,vmin);// go to 1	
         //         //}
                  double vv=smallgrid.getValue(i); 
                  smallgrid.setValue(i,ptr2obj->projectOuterLoop(vv));
 	 //         //if(dynamic_cast<BiasWeight*>(ptr2obj)){
	 //         //        smallgrid.addValue(i,-vmin);// bring back to the value	
         //         //}
         }

     return smallgrid; 
}
}
