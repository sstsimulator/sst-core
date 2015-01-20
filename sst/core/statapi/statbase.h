// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_CORE_STATISTICS_BASE
#define _H_SST_CORE_STATISTICS_BASE

#include <sst/core/sst_types.h>
#include <sst/core/serialization.h>

namespace SST {
class Component; 

namespace Statistics {
class StatisticOutput; 
    
/**
    \class StatisticBase

	Forms the base class for statistics gathering within SST. Statistics are
	gathered and processed into various (extensible) output forms. Statistics
	are expected to be named so that they can be located in the simulation 
	output files.
*/

class StatisticBase 
{
public:  
    /** Statistic collection mode */ 
    typedef enum {STAT_MODE_UNDEFINED, STAT_MODE_COUNT, STAT_MODE_PERIODIC} StatMode_t; 
    
    /** Construct a StatisticBase
      * @param comp - Pointer to the parent constructor.
      * @param statName - Name of the statistic to be registered.  This name must
      * match the name in the ElementInfoStatistic.
      */
    // Constructors:
    StatisticBase(Component* comp, std::string statName);
    StatisticBase(Component* comp, char* statName);
    // Destructor
    ~StatisticBase() {}
    
    /** Enable Statistic for collections */
    void enable() {m_statEnabled = true;}
    /** Disable Statistic for collections */
    void disable() {m_statEnabled = false;}
                 
    // Handling of Collection Counts
    /** Increment current collection count */
    virtual void incrementCollectionCount();  
    /** Set the current collection count to a defined value */
    virtual void setCollectionCount(uint64_t newCount);
    /** Set the current collection count to 0 */
    virtual void resetCollectionCount();
    /** Set the collection count limit to a defined value */
    virtual void setCollectionCountLimit(uint64_t newLimit);

    // Control Statistic Operation Flags
    /** Set the Reset Count On Output flag.
     *  If Set, the collection count will be reset when statistic is output. 
     */
    void setFlagResetCountOnOutput(bool flag) {m_resetCountOnOutput = flag;}

    /** Set the Clear Data On Output flag.
     *  If Set, the data in the statistic will be cleared by calling 
     *  clearStatisticData().
     */
    void setFlagClearDataOnOutput(bool flag) {m_clearDataOnOutput = flag;}

    /** Set the Output At End Of Sim flag.
     *  If Set, the statistic will perform an output at the end of simulation.
     */
    void setFlagOutputAtEndOfSim(bool flag) {m_outputAtEndOfSim = flag;}

    /** Set the Registered Collection Mode */
    void setRegisteredCollectionMode(StatMode_t mode) {m_registeredCollectionMode = mode;} 

    // Get Data
    /** Return the Component Name */
    const std::string& getCompName() const;
    /** Return the Statistic Name */
    inline const std::string& getStatName() const {return m_statName;}
    /** Return the full Component.Statistic Name */
    inline const std::string& getFullStatName() const {return m_fullName;}      // Return compname.statname
    /** Return a pointer to the parent Component */
    Component*   getComponent() const {return m_component;}
    /** Return the enable status of the Statistic */
    bool         isEnabled() const {return m_statEnabled;}
    /** Return the enable status of the Statistic's ability to output data */
    bool         isOutputEnabled() const {return m_outputEnabled;}
    /** Return the collection count limit */
    uint64_t     getCollectionCountLimit() const {return m_collectionCountLimit;}
    /** Return the current collection count */
    uint64_t     getCollectionCount() const {return m_currentCollectionCount;}
    /** Return the ResetCountOnOutput flag value */
    bool         getFlagResetCountOnOutput() const {return m_resetCountOnOutput;}
    /** Return the ClearDataOnOutput flag value */
    bool         getFlagClearDataOnOutput() const {return m_clearDataOnOutput;}
    /** Return the OutputAtEndOfSim flag value */
    bool         getFlagOutputAtEndOfSim() const {return m_outputAtEndOfSim;}
    /** Return the collection mode that is registered */
    StatMode_t   getRegisteredCollectionMode() const {return m_registeredCollectionMode;} 

    // Delay Methods (Uses OneShot to disable Statistic or Collection)
    /** Delay the statistic from outputting data for a specified delay time
      * @param delayTime - Value in UnitAlgebra format for delay (i.e. 10ns).
      */
    void  delayOutput(const char* delayTime);

    /** Delay the statistic from collecting data for a specified delay time.
     * @param delayTime - Value in UnitAlgebra format for delay (i.e. 10ns).
    */
    void delayCollection(const char* delayTime); 

    // Required Virtual Methods:
    /** Called by the system to tell the Statistic to register its output fields.
      * by calling statOutput->registerField(...)
      * @param statOutput - Pointer to the statistic output
      */
    virtual void registerOutputFields(StatisticOutput* statOutput) = 0;  

    /** Called by the system to tell the Statistic to send its data to the 
      * StatisticOutput to be output.
      * @param statOutput - Pointer to the statistic output
      * @param EndOfSimFlag - Indicates that the output is occuring at the end of simulation.
      */
    virtual void outputStatisticData(StatisticOutput* statOutput, bool EndOfSimFlag) = 0;

    /** Inform the Statistic to clear its data */
    virtual void clearStatisticData() {}      
    
    // Status of Statistic
    /** Indicate that the Statistic is Ready to be used */
    virtual bool isReady() const {return true;}     

    /** Indicate if the Statistic is a NullStatistic */
    virtual bool isNullStatistic() const {return false;} 

    /** Indicate if the Statistic Mode is supported.
      * This allows Statistics to suport STAT_MODE_COUNT and STAT_MODE_PERIODIC modes.
      * by default, both modes are supported.
      * @param mode - Mode to test
      */
    virtual bool isStatModeSupported(StatMode_t mode) const {return true;}      // Default is to accept all modes

    /** Verify that the statistic names match */
    bool operator==(StatisticBase& checkStat); 
    
private:
    // Support Routines
    void initializeProperties();
    void checkEventForOutput();

    // OneShot Callbacks:
    void delayOutputExpiredHandler();                               // Enable Output in handler
    void delayCollectionExpiredHandler();                           // Enable Collection in Handler

protected:     
    StatisticBase(); // For serialization only
    
private:
    Component*            m_component;
    std::string           m_statName;
    std::string           m_fullName;
    StatMode_t            m_registeredCollectionMode;
    uint64_t              m_currentCollectionCount;
    uint64_t              m_collectionCountLimit;
                          
    bool                  m_statEnabled;
    bool                  m_outputEnabled;
    bool                  m_resetCountOnOutput;
    bool                  m_clearDataOnOutput;
    bool                  m_outputAtEndOfSim;
                          
    bool                  m_outputDelayed;
    bool                  m_collectionDelayed;
    bool                  m_savedStatEnabled;
    bool                  m_savedOutputEnabled;
    OneShot::HandlerBase* m_outputDelayedHandler;
    OneShot::HandlerBase* m_collectionDelayedHandler;
    
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_NVP(m_component);
        ar & BOOST_SERIALIZATION_NVP(m_statName);
        ar & BOOST_SERIALIZATION_NVP(m_fullName);
        ar & BOOST_SERIALIZATION_NVP(m_registeredCollectionMode);
        ar & BOOST_SERIALIZATION_NVP(m_currentCollectionCount);
        ar & BOOST_SERIALIZATION_NVP(m_collectionCountLimit);
        ar & BOOST_SERIALIZATION_NVP(m_statEnabled);
        ar & BOOST_SERIALIZATION_NVP(m_outputEnabled);
        ar & BOOST_SERIALIZATION_NVP(m_resetCountOnOutput);
        ar & BOOST_SERIALIZATION_NVP(m_clearDataOnOutput);
        ar & BOOST_SERIALIZATION_NVP(m_outputAtEndOfSim);
        ar & BOOST_SERIALIZATION_NVP(m_outputDelayed);
        ar & BOOST_SERIALIZATION_NVP(m_collectionDelayed);
        ar & BOOST_SERIALIZATION_NVP(m_savedStatEnabled);
        ar & BOOST_SERIALIZATION_NVP(m_savedOutputEnabled);
        ar & BOOST_SERIALIZATION_NVP(m_outputDelayedHandler);
        ar & BOOST_SERIALIZATION_NVP(m_collectionDelayedHandler);
    }
};

////////////////////////////////////////////////////////////////////////////////

template <typename T>
class Statistic : public StatisticBase
{
public:    
    Statistic(Component* comp, std::string statName) :
        StatisticBase(comp, statName)
    {
    }
        
    Statistic(Component* comp, const char* statName) :
        StatisticBase(comp, statName)
    {
    }

    virtual ~Statistic(){}
    
    // The main method to add data to the statistic 
    void addData(T data)
    {
        // Call the Derived Statistic's implemenation 
        //  of addData and increment the count
        if (true == isEnabled()) {
            addData_impl(data);
            incrementCollectionCount();
        }
    }

protected:     
    Statistic(){}; // For serialization only

    // Required Templated Virtual Methods:
    virtual void addData_impl(T data) = 0;

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(StatisticBase);
    }
};

} //namespace Statistics
} //namespace SST

BOOST_CLASS_EXPORT_KEY(SST::Statistics::StatisticBase)
BOOST_CLASS_EXPORT_KEY(SST::Statistics::Statistic<uint32_t>)

#endif
