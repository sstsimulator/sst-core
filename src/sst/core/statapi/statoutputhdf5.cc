// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <algorithm>

#include <sst/core/simulation.h>
#include <sst/core/warnmacros.h>
#include <sst/core/baseComponent.h>
#include <sst/core/statapi/statoutputhdf5.h>
#include <sst/core/statapi/statgroup.h>
#include <sst/core/stringize.h>

namespace SST {
namespace Statistics {

StatisticOutputHDF5::StatisticOutputHDF5(Params& outputParameters)
    : StatisticOutput (outputParameters),
    m_hFile(NULL),
    m_currentDataSet(NULL)
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
}


void StatisticOutputHDF5::implStartRegisterFields(StatisticBase *stat)
{
    if ( m_currentDataSet != NULL ) {
        m_currentDataSet->setCurrentStatistic(stat);
    } else {
        m_currentDataSet = initStatistic(stat);
    }
}


void StatisticOutputHDF5::implRegisteredField(fieldHandle_t fieldHandle)
{
    StatisticFieldInfo* fi = getRegisteredField(fieldHandle);
    m_currentDataSet->registerField(fi);
}

void StatisticOutputHDF5::implStopRegisterFields()
{
    m_currentDataSet->finalizeCurrentStatistic();
    if ( !m_currentDataSet->isGroup() )
        m_currentDataSet = NULL;
}


void StatisticOutputHDF5::implStartRegisterGroup(StatisticGroup* group )
{
    m_statGroups.emplace(std::piecewise_construct,
            std::forward_as_tuple(group->name),
            std::forward_as_tuple(group, m_hFile));
    m_currentDataSet = &m_statGroups.at(group->name);
    m_currentDataSet->beginGroupRegistration(group);
}


void StatisticOutputHDF5::implStopRegisterGroup()
{
    m_currentDataSet->finalizeGroupRegistration();
    m_currentDataSet = NULL;
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
    if ( m_currentDataSet == NULL )
        m_currentDataSet = getStatisticInfo(statistic);
    m_currentDataSet->startNewEntry(statistic);
}

void StatisticOutputHDF5::implStopOutputEntries()
{
    m_currentDataSet->finishEntry();
    if ( !m_currentDataSet->isGroup() )
        m_currentDataSet = NULL;
}



void StatisticOutputHDF5::implStartOutputGroup(StatisticGroup* group)
{
    m_currentDataSet = &m_statGroups.at(group->name);
    m_currentDataSet->startNewGroupEntry();
}


void StatisticOutputHDF5::implStopOutputGroup()
{
    m_currentDataSet->finishGroupEntry();
    m_currentDataSet = NULL;
}





void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, int32_t data)
{
    m_currentDataSet->getFieldLoc(fieldHandle).i32 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, uint32_t data)
{
    m_currentDataSet->getFieldLoc(fieldHandle).u32 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, int64_t data)
{
    m_currentDataSet->getFieldLoc(fieldHandle).i64 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, uint64_t data)
{
    m_currentDataSet->getFieldLoc(fieldHandle).u64 = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, float data)
{
    m_currentDataSet->getFieldLoc(fieldHandle).f = data;
}

void StatisticOutputHDF5::implOutputField(fieldHandle_t fieldHandle, double data)
{
    m_currentDataSet->getFieldLoc(fieldHandle).d = data;
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





void StatisticOutputHDF5::StatisticInfo::startNewEntry(StatisticBase *UNUSED(stat))
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



void StatisticOutputHDF5::StatisticInfo::registerField(StatisticFieldInfo *fi)
{
    typeList.push_back(fi->getFieldType());
    indexMap.push_back(fi->getFieldHandle());
    fieldNames.push_back(fi->getFieldName());
}




static H5::DataType getMemTypeForStatType(StatisticOutput::fieldType_t type) {
    switch ( type ) {
    default:
    case StatisticFieldInfo::UNDEFINED:
        Output::getDefaultObject().fatal(CALL_INFO, -1, "Unhandled UNDEFINED datatype.\n");
        return H5::PredType::NATIVE_UINT32;
    case StatisticFieldInfo::UINT32: return H5::PredType::NATIVE_UINT32;
    case StatisticFieldInfo::UINT64: return H5::PredType::NATIVE_UINT64;
    case StatisticFieldInfo::INT32:  return H5::PredType::NATIVE_INT32;
    case StatisticFieldInfo::INT64:  return H5::PredType::NATIVE_INT64;
    case StatisticFieldInfo::FLOAT:  return H5::PredType::NATIVE_FLOAT;
    case StatisticFieldInfo::DOUBLE: return H5::PredType::NATIVE_DOUBLE;
    }
}


void StatisticOutputHDF5::StatisticInfo::finalizeCurrentStatistic()
{
    size_t nFields = typeList.size();
    currentData.resize(nFields);

    /* Build HDF5 datatypes */
    size_t dataSize = currentData.size() * sizeof(StatData_u);
    memType = new H5::CompType(dataSize);
    for ( size_t i = 0 ; i < nFields ; i++ ) {
        const std::string &name = fieldNames[i];
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
    hsize_t chunk_dims[1] = {1024};
    cparms.setChunk(1, chunk_dims);
    cparms.setDeflate(7);

    dataset = new H5::DataSet( file->createDataSet(statName, *memType, dspace, cparms) );


    typeList.clear();
    fieldNames.clear();
}




StatisticOutputHDF5::GroupInfo::GroupInfo(StatisticGroup *group, H5::H5File *file) :
    DataSet(file), nEntries(0), m_statGroup(group)
{
    /* We need to store component pointers, not just IDs */
    m_components.resize(m_statGroup->components.size());

    /* Create group directory */
    std::string objName = "/" + getName();
    try {
        H5::Group* compGroup = new H5::Group( file->createGroup(objName));
        compGroup->close();
        delete compGroup;
    } catch (H5::FileIException ex) {
        /* Ignore - group already exists.*/
    }

}



void StatisticOutputHDF5::GroupInfo::setCurrentStatistic(StatisticBase *stat)
{
    std::string statName = GroupStat::getStatName(stat);
    if ( m_statGroups.find(statName) == m_statGroups.end() ) {
        m_statGroups.emplace(std::piecewise_construct,
            std::forward_as_tuple(statName),
            std::forward_as_tuple(this, stat));
    }
    m_currentStat = &(m_statGroups.at(statName));


    /* Find and set in our m_components vector */
    ComponentId_t id = stat->getComponent()->getId();
    for ( size_t i = 0 ; i < m_statGroup->components.size() ; i++ ) {
        if ( m_statGroup->components.at(i) == id ) {
            m_components.at(i) = stat->getComponent();
        }
    }
}


void StatisticOutputHDF5::GroupInfo::registerField(StatisticFieldInfo *fi)
{
    size_t index = 0;
    std::string name = fi->getFieldUniqueName();

    auto location = std::find(
            m_currentStat->registeredFields.begin(),
            m_currentStat->registeredFields.end(),
            name);
    bool firstSeen = (location == m_currentStat->registeredFields.end());

    if ( firstSeen ) {
        index = m_currentStat->registeredFields.size();
        m_currentStat->registeredFields.push_back(name);
        m_currentStat->typeList.push_back(fi->getFieldType());
    } else {
        index = std::distance(m_currentStat->registeredFields.begin(), location);
    }
    m_currentStat->handleIndexMap[fi->getFieldHandle()] = index;
}



void StatisticOutputHDF5::GroupInfo::finalizeCurrentStatistic()
{
    m_currentStat = NULL;
}


void StatisticOutputHDF5::GroupInfo::finalizeGroupRegistration()
{
    for ( auto &stat : m_statGroups ) {
        stat.second.finalizeRegistration();
    }
    /* Create:
     *      /group/components
     *          Arrays of Component information
     *          /ids
     *          /names
     *          /coord_x
     *          /coord_y
     *          /coord_z
     *      /group/timestamp
     *          Array of timestamps for each entry
     */


    /* Create components sub-group */
    std::string groupName = "/" + getName() + "/components";
    try {
        H5::Group* statGroup = new H5::Group( getFile()->createGroup(groupName) );
        statGroup->close();
        delete statGroup;
    } catch (H5::FileIException ie) {
        /* Ignore - group already exists. */
    }

    H5::DSetCreatPropList cparms;
    hsize_t chunk_dims[1] = {std::min(m_statGroup->components.size(), (size_t)64)};
    cparms.setChunk(1, chunk_dims);
    cparms.setDeflate(7);



    /* Create arrays */
    hsize_t infoDim[1] = {m_statGroup->components.size()};
    H5::DataSpace infoSpace(1, infoDim);
    H5::DataSet *idSet = new H5::DataSet(getFile()->createDataSet(groupName + "/ids", H5::PredType::NATIVE_UINT64, infoSpace, cparms));
    H5::DataSet *nameSet = new H5::DataSet(getFile()->createDataSet(groupName + "/names", H5::StrType(H5::PredType::C_S1, H5T_VARIABLE), infoSpace, cparms));
    H5::DataSet *coordXSet = new H5::DataSet(getFile()->createDataSet(groupName + "/coord_x", H5::PredType::NATIVE_DOUBLE, infoSpace, cparms));
    H5::DataSet *coordYSet = new H5::DataSet(getFile()->createDataSet(groupName + "/coord_y", H5::PredType::NATIVE_DOUBLE, infoSpace, cparms));
    H5::DataSet *coordZSet = new H5::DataSet(getFile()->createDataSet(groupName + "/coord_z", H5::PredType::NATIVE_DOUBLE, infoSpace, cparms));

    std::vector<uint64_t> idVec;
    std::vector<const char*> nameVec;
    std::vector<double> xVec;
    std::vector<double> yVec;
    std::vector<double> zVec;

    for ( size_t i = 0 ; i < infoDim[0] ; i++ ) {
        BaseComponent* comp = m_components.at(i);

        idVec.push_back(comp->getId());
        nameVec.push_back(comp->getName().c_str());
        const std::vector<double> &coords = comp->getCoordinates();
        xVec.push_back(coords[0]);
        yVec.push_back(coords[1]);
        zVec.push_back(coords[2]);
    }

    idSet->write(idVec.data(), H5::PredType::NATIVE_UINT64);
    nameSet->write(nameVec.data(), H5::StrType(H5::PredType::C_S1, H5T_VARIABLE));
    coordXSet->write(xVec.data(), H5::PredType::NATIVE_DOUBLE);
    coordYSet->write(yVec.data(), H5::PredType::NATIVE_DOUBLE);
    coordZSet->write(zVec.data(), H5::PredType::NATIVE_DOUBLE);

    delete idSet;
    delete nameSet;
    delete coordXSet;
    delete coordYSet;
    delete coordZSet;

    /* Create timestamp array */
    hsize_t tdim[1] = {0};
    hsize_t maxdims[1] = { H5S_UNLIMITED };
    H5::DataSpace tspace(1, tdim, maxdims);
    timeDataSet = new H5::DataSet(getFile()->createDataSet("/" + getName() + "/timestamps", H5::PredType::NATIVE_UINT64, tspace, cparms));

}








void StatisticOutputHDF5::GroupInfo::startNewGroupEntry() {
    /* Record current timestamp */
    for ( auto & gs : m_statGroups ) {
        gs.second.startNewGroupEntry();
    }

    hsize_t dims[1] = {1};
    hsize_t offset[1] = { nEntries };

    hsize_t newSize[1] = { ++nEntries };
    timeDataSet->extend(newSize);

    H5::DataSpace fspace = timeDataSet->getSpace();
    H5::DataSpace memSpace( 1, dims );
    fspace.selectHyperslab( H5S_SELECT_SET, dims, offset );
    uint64_t currTime = Simulation::getSimulation()->getCurrentSimCycle();
    timeDataSet->write(&currTime, H5::PredType::NATIVE_UINT64, memSpace, fspace);
}



void StatisticOutputHDF5::GroupInfo::startNewEntry(StatisticBase *stat)
{
    m_currentStat = &(m_statGroups.at(GroupStat::getStatName(stat)));
    size_t compIndex = std::distance(m_components.begin(),
            std::find(m_components.begin(), m_components.end(), stat->getComponent()));
    m_currentStat->startNewEntry(compIndex, stat);
}


void StatisticOutputHDF5::GroupInfo::finishEntry()
{
    m_currentStat->finishEntry();
    m_currentStat = NULL;
}

void StatisticOutputHDF5::GroupInfo::finishGroupEntry()
{
    for ( auto & gs : m_statGroups ) {
        gs.second.finishGroupEntry();
    }
}




std::string StatisticOutputHDF5::GroupInfo::GroupStat::getStatName(StatisticBase* stat)
{
    if ( stat->getStatSubId().empty() )
        return stat->getStatName();
    return stat->getStatName() + "." + stat->getStatSubId();
}

StatisticOutputHDF5::GroupInfo::GroupStat::GroupStat(GroupInfo* group, StatisticBase* stat) :
    gi(group), nEntries(0)
{

    /* Create the file hierarchy */
    statPath = "/" + group->getName() + "/" + stat->getStatName();
    if ( stat->getStatSubId().length() > 0 ) {
        try {
            H5::Group* statGroup = new H5::Group( group->getFile()->createGroup(statPath));
            statGroup->close();
            delete statGroup;
        } catch (H5::FileIException ie) {
            /* Ignore - group already exists. */
        }
        statPath += "/" + stat->getStatSubId();
    }

}

void StatisticOutputHDF5::GroupInfo::GroupStat::finalizeRegistration()
{
    size_t nslots = registeredFields.size();
    currentData.resize(nslots * gi->getNumComponents());

    /* Build a HDF5 in-Memory datatype */
    size_t dataSize = nslots * sizeof(StatData_u);
    memType = new H5::CompType(dataSize);
    for ( size_t i = 0 ; i < nslots ; i++ ) {
        H5::DataType type = getMemTypeForStatType(typeList[i]);
        memType->insertMember(registeredFields.at(i), i*sizeof(StatData_u), type);
    }
    typeList.clear();

    hsize_t dims[2] = {gi->getNumComponents(), 0};
    hsize_t maxdims[2] = {gi->getNumComponents(), H5S_UNLIMITED};

    H5::DataSpace dspace(2, dims, maxdims);
    H5::DSetCreatPropList cparms;
    hsize_t chunk_dims[2] = {std::min((hsize_t)16, dims[0]), 128};
    cparms.setChunk(2, chunk_dims);
    cparms.setDeflate(7);

    dataset = new H5::DataSet( gi->getFile()->createDataSet(statPath, *memType, dspace, cparms) );
}




void StatisticOutputHDF5::GroupInfo::GroupStat::startNewGroupEntry() {
    for ( StatData_u &i : currentData ) {
        memset(&i, '\0', sizeof(i));
    }
}


void StatisticOutputHDF5::GroupInfo::GroupStat::startNewEntry(size_t componentIndex, StatisticBase *UNUSED(stat))
{
    currentCompOffset = componentIndex * registeredFields.size();
}


StatisticOutputHDF5::StatData_u& StatisticOutputHDF5::GroupInfo::GroupStat::getFieldLoc(fieldHandle_t fieldHandle)
{
    size_t fieldIndex = handleIndexMap.at(fieldHandle);
    return currentData[currentCompOffset + fieldIndex];
}


void StatisticOutputHDF5::GroupInfo::GroupStat::finishEntry()
{
}


void StatisticOutputHDF5::GroupInfo::GroupStat::finishGroupEntry()
{
    /* TODO */
    hsize_t dims[2] = {gi->getNumComponents(), 1};
    hsize_t offset[2] = { 0, nEntries };

    hsize_t newSize[2] = { gi->getNumComponents(), ++nEntries };
    dataset->extend(newSize);

    H5::DataSpace fspace = dataset->getSpace();
    H5::DataSpace memSpace( 2, dims );
    fspace.selectHyperslab( H5S_SELECT_SET, dims, offset );
    dataset->write(currentData.data(), *memType, memSpace, fspace);
}


} //namespace Statistics
} //namespace SST
