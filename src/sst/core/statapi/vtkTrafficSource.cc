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
  Module:    vtkTrafficSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkTrafficSource.h"

#include "vtkObjectFactory.h"
#include "vtkAlgorithm.h"
#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include <vector>

#include "vtkExodusIIWriter.h"
#include <vtkIntArray.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkHexahedron.h>
#include <vtkUnstructuredGrid.h>

#include "sst/core/statapi/statintensity.h"
#include "sst/core/simulation.h"


vtkStandardNewMacro(vtkTrafficSource);

//----------------------------------------------------------------------------
vtkTrafficSource::vtkTrafficSource()
{
    this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkTrafficSource::~vtkTrafficSource()
{
}

void vtkTrafficSource::SetNumberOfSteps(double count)
{
    this->NumSteps_ = count;
}
void vtkTrafficSource::SetSteps(double *steps)
{
    this->Steps_ = steps;
}

// Topology
void vtkTrafficSource::SetPoints(vtkSmartPointer<vtkPoints> points)
{
    this->Points = points;
}

void vtkTrafficSource::SetCells(vtkSmartPointer<vtkCellArray> cells)
{
    this->Cells = cells;
}

// Traffic
void vtkTrafficSource::SetTraffics(vtkSmartPointer<vtkIntArray> traffics)
{
    this->Traffics = traffics;
}

//----------------------------------------------------------------------------
int vtkTrafficSource::RequestInformation(
  vtkInformation* reqInfo,
  vtkInformationVector** inVector,
  vtkInformationVector* outVector
  )
{
    if(!this->Superclass::RequestInformation(reqInfo,inVector,outVector))
    {
      return 0;
    }

    vtkInformation *info=outVector->GetInformationObject(0);

    //tell the caller that I can provide time varying data and
    //tell it what range of times I can deal with
    double tRange[2];
    tRange[0] = this->Steps_[0];
    tRange[1] = this->Steps_[this->NumSteps_-1];
    info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
    tRange,
    2);

    //tell the caller if this filter can provide values ONLY at discrete times
    //or anywhere within the time range

    info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
    this->Steps_,
    this->NumSteps_);


    info->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

    return 1;
}

//----------------------------------------------------------------------------
int vtkTrafficSource::RequestData(
  vtkInformation* vtkNotUsed(reqInfo),
  vtkInformationVector** vtkNotUsed(inVector),
  vtkInformationVector* outVector
  )
{
    static int timestep = 0;

    vtkInformation *outInfo = outVector->GetInformationObject(0);
    vtkUnstructuredGrid *output= vtkUnstructuredGrid::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));
    if (!output)
    {
      return 0;
    }

    uint64_t reqTS(0);
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
        double requested = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
        reqTS = llround(requested);
    }

    //if analytic compute the value at that time
    //TODO: check if it's necessary to look up the nearest time and value from the table
    output->Initialize();
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(), reqTS);

    //Updade and Send traffic to output
    auto currentIntensities =  traffic_progress_map_.equal_range(reqTS);

    for(auto it = currentIntensities.first; it != currentIntensities.second; ++it){
        auto& event = it->second;
        this->Traffics->SetValue(event.cellId_, event.ie_.intensity_);
    }
    ++timestep;

    output->GetCellData()->AddArray(this->Traffics);

    // Send Topology to output
    output->SetPoints(this->Points);
    output->SetCells(this->CellTypes.data(), this->Cells);

    return 1;
}

//----------------------------------------------------------------------------
void vtkTrafficSource::PrintSelf(ostream& os, vtkIndent indent)
{
    this->Superclass::PrintSelf(os, indent);
}

void vtkTrafficSource::vtkOutputExodus(const std::string& fileroot,
    std::multimap<uint64_t, SST::Statistics::sorted_intensity_event>&& traffMap,
    std::vector<SST::Statistics::Stat3DViz>&& stat3dVizVector)
{
    static constexpr int NUM_POINTS_PER_BOX = 8;
    static constexpr int NUM_POINTS_PER_LINK = 2;

    // Create the vtkPoints
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();

    // Compute the number of the points
    int numberOfPoints = 0;
    for (const auto& stat3dViz : stat3dVizVector) {
      switch (stat3dViz.my_shape_->shape) {
      case SST::Statistics::Shape3D::Box: {
          numberOfPoints += NUM_POINTS_PER_BOX;
          break;
      }
      case SST::Statistics::Shape3D::Line: {
          numberOfPoints += NUM_POINTS_PER_LINK;
          break;
      }
      default: {
           SST::Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot compute the number of points: "                                                                              "Unknown Shape3D type detected\n");
      }
      }
    }

    points->SetNumberOfPoints(numberOfPoints);

    // Create the vtkCellArray
    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();

    std::vector<int> cell_types;
    cell_types.reserve(stat3dVizVector.size());

    int i = 0;
    int cellId = 0;
    for (const auto& stat3dViz : stat3dVizVector) {
        SST::Statistics::Shape3D *shape = stat3dViz.my_shape_;
        switch (stat3dViz.my_shape_->shape) {
        case SST::Statistics::Shape3D::Box: {
            SST::Statistics::Box3D * box = static_cast<SST::Statistics::Box3D*> (shape);
            // Fill the vtkPoints
            points->SetPoint(0 + i, box->origin_[0], box->origin_[1], box->origin_[2]);
            points->SetPoint(1 + i, box->origin_[0] + box->size_[0], box->origin_[1], box->origin_[2]);
            points->SetPoint(2 + i, box->origin_[0] + box->size_[0], box->origin_[1] + box->size_[1], box->origin_[2]);
            points->SetPoint(3 + i, box->origin_[0], box->origin_[1] + box->size_[1], box->origin_[2]);
            points->SetPoint(4 + i, box->origin_[0], box->origin_[1], box->origin_[2] + box->size_[2]);
            points->SetPoint(5 + i, box->origin_[0] + box->size_[0], box->origin_[1], box->origin_[2] + box->size_[2]);
            points->SetPoint(6 + i, box->origin_[0] + box->size_[0], box->origin_[1] + box->size_[1], box->origin_[2] + box->size_[2]);
            points->SetPoint(7 + i, box->origin_[0], box->origin_[1] + box->size_[1], box->origin_[2] + box->size_[2]);
            // Fill the vtkCellArray
            vtkSmartPointer<vtkHexahedron> cell = vtkSmartPointer<vtkHexahedron>::New();
            for (int j= 0; j< NUM_POINTS_PER_BOX; ++j) {
                cell->GetPointIds()->SetId(j, i + j);
            }
            cells->InsertNextCell(cell);
            cell_types.push_back(VTK_HEXAHEDRON);

            i += NUM_POINTS_PER_BOX;
            break;
        }
        case SST::Statistics::Shape3D::Line: {
            SST::Statistics::Line3D * line = static_cast<SST::Statistics::Line3D*> (shape);
            // Fill the vtkPoints
            points->SetPoint(0 + i, line->origin_[0], line->origin_[1], line->origin_[2]);
            points->SetPoint(1 + i, line->origin_[0] + line->size_[0], line->origin_[1] + line->size_[1], line->origin_[2] + line->size_[2]);

            // Fill the cells
            vtkSmartPointer<vtkLine> cell = vtkSmartPointer<vtkLine>::New();
            for (int j= 0; j< NUM_POINTS_PER_LINK; ++j) {
                cell->GetPointIds()->SetId(j, i + j);
            }
            cells->InsertNextCell(cell);
            cell_types.push_back(VTK_LINE);

            i += NUM_POINTS_PER_LINK;
            break;
         }
        default: {
            SST::Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, 1, "Cannot display the geometry: "                                                                                   "Unknown Shape3D type detected\n");
        }
        }
        cellId += 1;
    }

    // Init traffic array with default 0 traffic value
    vtkSmartPointer<vtkIntArray> traffic = vtkSmartPointer<vtkIntArray>::New();
    traffic->SetNumberOfComponents(1);
    traffic->SetName("MyTraffic");
    traffic->SetNumberOfValues(cells->GetNumberOfCells());

    for (int c = 0; c < cells->GetNumberOfCells(); ++c) {
        traffic->SetValue(c,0);
    }

    vtkSmartPointer<vtkUnstructuredGrid> unstructured_grid =
      vtkSmartPointer<vtkUnstructuredGrid>::New();

    unstructured_grid->SetPoints(points);
    unstructured_grid->SetCells(cell_types.data(), cells);
    unstructured_grid->GetCellData()->AddArray(traffic);

    // Init Time Step
    double current_time = -1;
    double *time_step_value = new double[traffMap.size() + 1];
    time_step_value[0] = 0.;
    int currend_index = 1;
    for (auto it = traffMap.cbegin(); it != traffMap.cend(); ++it){
        if (it->first != current_time){
            current_time = it->first;
            time_step_value[currend_index] = it->first;
            ++currend_index;
        }
    }

    vtkSmartPointer<vtkTrafficSource> trafficSource = vtkSmartPointer<vtkTrafficSource>::New();
    trafficSource->SetTrafficProgressMap(std::move(traffMap));
    trafficSource->SetTraffics(traffic);
    trafficSource->SetPoints(points);
    trafficSource->SetCells(cells);
    trafficSource->SetSteps(time_step_value);
    trafficSource->SetNumberOfSteps(currend_index);
    trafficSource->SetCellTypes(std::move(cell_types));

    vtkSmartPointer<vtkExodusIIWriter> exodusWriter = vtkSmartPointer<vtkExodusIIWriter>::New();
    std::string fileName = fileroot;
    if(fileroot.find(".e") ==  std::string::npos) {
        fileName = fileroot + ".e";
    }
    exodusWriter->SetFileName(fileName.c_str());
    exodusWriter->SetInputConnection (trafficSource->GetOutputPort());
    exodusWriter->WriteAllTimeStepsOn ();
    exodusWriter->Write();
}
