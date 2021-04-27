// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTrafficSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTimeSource
 * @brief   creates a simple time varying data set.
 *
 * Creates a small easily understood time varying data set for testing.
 * The output is a vtkUntructuredGrid in which the point and cell values vary
 * over time in a sin wave. The analytic ivar controls whether the output
 * corresponds to a step function over time or is continuous.
 * The X and Y Amplitude ivars make the output move in the X and Y directions
 * over time. The Growing ivar makes the number of cells in the output grow
 * and then shrink over time.
*/

#ifndef vtkTrafficSource_h
#define vtkTrafficSource_h

#include "vtkUnstructuredGridAlgorithm.h"

#include "vtkCellArray.h"
#include "vtkIntArray.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "statintensity.h"

using namespace SST::Statistics;

class vtkTrafficSource : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkTrafficSource *New();
  vtkTypeMacro(vtkTrafficSource,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void SetNumberOfSteps(double count);
  void SetSteps(double *steps);

  // Topology
  void SetPoints(vtkSmartPointer<vtkPoints> points);
  void SetCells(vtkSmartPointer<vtkCellArray> cells);

  template <class Vec> //allow move or copy
  void SetCellTypes(Vec&& types){
      CellTypes = std::forward<Vec>(types);
  }

  // Traffic
  void SetTrafficProgressMap(std::multimap<uint64_t, SST::Statistics::sorted_intensity_event>&& trafficProgressMap){
      traffic_progress_map_ = std::move(trafficProgressMap);
  }

  void SetTraffics(vtkSmartPointer<vtkIntArray> traffics);


  static void vtkOutputExodus(const std::string& fileroot,
        std::multimap<uint64_t, SST::Statistics::sorted_intensity_event>&& traffMap,
        std::vector<SST::Statistics::Stat3DViz>&& stat3dVizVector);


protected:
  vtkTrafficSource();
  ~vtkTrafficSource() override;

  int RequestInformation(vtkInformation*,
                         vtkInformationVector**,
                         vtkInformationVector*) override;

  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*) override;

  int NumSteps_;
  double *Steps_;
  std::multimap<uint64_t, SST::Statistics::sorted_intensity_event> traffic_progress_map_;
  vtkSmartPointer<vtkIntArray> Traffics;
  vtkSmartPointer<vtkPoints> Points;
  vtkSmartPointer<vtkCellArray> Cells;
  std::vector<int> CellTypes;

private:
  vtkTrafficSource(const vtkTrafficSource&) = delete;
  void operator=(const vtkTrafficSource&) = delete;
};

#endif

