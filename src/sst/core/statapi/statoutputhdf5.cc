// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/simulation.h>
#include <sst/core/statapi/statoutputhdf5.h>
#include <sst/core/stringize.h>

namespace SST {
namespace Statistics {

StatisticOutputHDF5::StatisticOutputHDF5(Params& outputParameters)
    : StatisticOutput (outputParameters)
{
    // Announce this output object's name
    Output &out = Simulation::getSimulationOutput();
    out.verbose(CALL_INFO, 1, 0, " : StatisticOutputHDF5 enabled...\n");
    setStatisticOutputName("StatisticOutputHDF5");
}

bool StatisticOutputHDF5::checkOutputParameters()
{
    bool foundKey;

    // Review the output parameters and make sure they are correct, and 
    // also setup internal variables

    // Look for Help Param
    getOutputParameters().find<std::string>("help", "1", foundKey);
    if (true == foundKey) {
        return false;
    }

    // Get the parameters
    std::string m_filePath = getOutputParameters().find<std::string>("filepath", "./StatisticOutput.h5");

    if (0 == m_filePath.length()) {
        // Filepath is zero length
        return false;
    }

    H5::Exception::dontPrint();

    m_hFile = new H5::H5File(m_filePath, H5F_ACC_TRUNC);

    return true;
}

void StatisticOutputHDF5::printUsage()
{
    // Display how to use this output object
    Output out("", 0, 0, Output::STDOUT);
    out.output(" : Usage - Sends all statistic output to a HDF5 File.\n");
    out.output(" : Parameters:\n");
    out.output(" : help = Force Statistic Output to display usage\n");
    out.output(" : filepath = <Path to .h5 file> - Default is ./StatisticOutput.h5\n");
    out.output(" : outputsimtime = 0 | 1 - Output Simulation Time - Default is 1\n");
}


void StatisticOutputHDF5::implStartRegisterFields(StatisticBase *stat)
{
    m_currentStatistic = initStatistic(stat);
    m_currentStatistic->registerSimTime();
}


void StatisticOutputHDF5::implRegisteredField(fieldHandle_t fieldHandle)
{
    StatisticFieldInfo* fi = getRegisteredField(fieldHandle);
    m_currentStatistic->registerField(fi);
}

void StatisticOutputHDF5::implStopRegisterFields()
{
    m_currentStatistic->finalizeRegistration();
    m_currentStatistic = NULL;
}



void StatisticOutputHDF5::startOfSimulation()
{
}


void StatisticOutputHDF5::endOfSimulation()
{
    for ( auto i : m_statistics ) {
        delete i.second;
    }
    delete m_hFile;
}

void StatisticOutputHDF5::implStartOutputEntries(StatisticBase* statistic)
{
    m_currentStatistic = getStatisticInfo(statistic);
    m_currentStatistic->startNewEntry();
}

void StatisticOutputHDF5::implStopOutputEntries()
{
    m_currentStatistic->finishEntry();
}



void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, int32_t data)
{
    m_currentStatistic->getFieldLoc(fieldHandle).i32 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, uint32_t data)
{
    m_currentStatistic->getFieldLoc(fieldHandle).u32 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, int64_t data)
{
    m_currentStatistic->getFieldLoc(fieldHandle).i64 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, uint64_t data)
{
    m_currentStatistic->getFieldLoc(fieldHandle).u64 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, float data)
{
    m_currentStatistic->getFieldLoc(fieldHandle).f = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, double data)
{
    m_currentStatistic->getFieldLoc(fieldHandle).d = data;
}



StatisticOutputHDF5::StatisticInfo* StatisticOutputHDF5::initStatistic(StatisticBase* statistic)
{
    StatisticInfo *si = new StatisticInfo(statistic, m_hFile);
    m_statistics[statistic] = si;
    return si;
}


StatisticOutputHDF5::StatisticInfo* StatisticOutputHDF5::getStatisticInfo(StatisticBase* statistic)
{
    return m_statistics.at(statistic);
}





void StatisticOutputHDF5::StatisticInfo::startNewEntry()
{
    for ( StatData_u &i : currentData ) {
        memset(&i, '\0', sizeof(i));
    }
    currentData[0].u64 = Simulation::getSimulation()->getCurrentSimCycle();
}


StatisticOutputHDF5::StatData_u& StatisticOutputHDF5::StatisticInfo::getFieldLoc(fieldHandle_t fieldHandle)
{
    size_t nItems = indexMap.size();
    for ( size_t i = 0 ; i < nItems ; i++ ) {
        if ( indexMap[i] == fieldHandle )
            return currentData[i];
    }
    Output::getDefaultObject().fatal(CALL_INFO, -1, "Attempting to access unregistered Field Handle\n");
    // Not reached
    return currentData[0];
}


void StatisticOutputHDF5::StatisticInfo::finishEntry()
{
    hsize_t dims[1] = {1};
    hsize_t offset[1] = { nEntries };

    hsize_t newSize[1] = { ++nEntries };
    dataset->extend(newSize);

    H5::DataSpace fspace = dataset->getSpace();
    H5::DataSpace memSpace( 1, dims );
    fspace.selectHyperslab( H5S_SELECT_SET, dims, offset );
    dataset->write(currentData.data(), *memType, memSpace, fspace);

}


void StatisticOutputHDF5::StatisticInfo::registerSimTime()
{
    typeList.push_back(StatisticFieldInfo::UINT64);
    indexMap.push_back(-1);
}


void StatisticOutputHDF5::StatisticInfo::registerField(StatisticFieldInfo *fi)
{
    typeList.push_back(fi->getFieldType());
    indexMap.push_back(fi->getFieldHandle());
    fieldNames.push_back(fi->getFieldName());
}




static H5::DataType getMemTypeForStatType(StatisticOutput::fieldType_t type) {
    switch ( type ) {
    case StatisticFieldInfo::UNDEFINED:
        Output::getDefaultObject().fatal(CALL_INFO, -1, "Unhandled UNDEFINED datatype.\n");
        return NULL;
    case StatisticFieldInfo::UINT32: return H5::PredType::NATIVE_UINT32;
    case StatisticFieldInfo::UINT64: return H5::PredType::NATIVE_UINT64;
    case StatisticFieldInfo::INT32:  return H5::PredType::NATIVE_INT32;
    case StatisticFieldInfo::INT64:  return H5::PredType::NATIVE_INT64;
    case StatisticFieldInfo::FLOAT:  return H5::PredType::NATIVE_FLOAT;
    case StatisticFieldInfo::DOUBLE: return H5::PredType::NATIVE_DOUBLE;
    }
}


void StatisticOutputHDF5::StatisticInfo::finalizeRegistration()
{
    size_t nFields = typeList.size() -1;
    currentData.resize(nFields);

    /* Build HDF5 datatypes */
    size_t dataSize = currentData.size() * sizeof(StatData_u);
    memType = new H5::CompType(dataSize);
    for ( size_t i = 0 ; i < nFields ; i++ ) {
        const std::string &name = (indexMap[i] == -1)
            ? "SimTime"
            : fieldNames[i];
        size_t offset = (char*)&currentData[i] - (char*)&currentData[0];
        H5::DataType type = getMemTypeForStatType(typeList[i]);
        memType->insertMember(name, offset, type);
    }



    /* Create the file hierarchy */
    std::string objName = "/" + statistic->getCompName();
    try {
        H5::Group* compGroup = new H5::Group( file->createGroup(objName));
        compGroup->close();
        delete compGroup;
    } catch (H5::FileIException ex) {
        /* Ignore - group already exists.*/
    }

    std::string statName = objName + "/" + statistic->getStatName();

    if ( statistic->getStatSubId().length() > 0 ) {
        try {
            H5::Group* statGroup = new H5::Group( file->createGroup(statName));
            statGroup->close();
            delete statGroup;
        } catch (H5::FileIException ie) {
            /* Ignore - group already exists. */
        }
        statName += "/" + statistic->getStatSubId();
    }


    /* Create dataspace & Dataset */
    hsize_t dims[1] = {0};
    hsize_t maxdims[1] = { H5S_UNLIMITED };
    H5::DataSpace dspace(1, dims, maxdims);
    H5::DSetCreatPropList cparms;
    hsize_t chunk_dims[1] = {128};
    cparms.setChunk(1, chunk_dims);
    cparms.setDeflate(7);

    dataset = new H5::DataSet( file->createDataSet(statName, *memType, dspace, cparms) );


    typeList.clear();
    fieldNames.clear();
}


} //namespace Statistics
} //namespace SST
