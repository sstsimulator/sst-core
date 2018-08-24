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

#ifndef _H_SST_CORE_STATISTICS_BASE
#define _H_SST_CORE_STATISTICS_BASE

#include <string>

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>
#include <sst/core/params.h>
#include <sst/core/oneshot.h>
#include <sst/core/statapi/statfieldinfo.h>
#include <sst/core/elementinfo.h>
#include <sst/core/serialization/serializable.h>

namespace SST {
class BaseComponent;
class Factory;
namespace Statistics {
class StatisticOutput;
class StatisticProcessingEngine;
class StatisticGroup;


class StatisticInfo : public SST::Core::Serialization::serializable {
public:
    std::string name;
    Params params;

    StatisticInfo(const std::string &name) : name(name) { }
    StatisticInfo(const std::string &name, const Params &params) : name(name), params(params) { }
    StatisticInfo() { } /* DO NOT USE:  For serialization */

    void serialize_order(SST::Core::Serialization::serializer &ser) override {
        ser & name;
        ser & params;
    }

    ImplementSerializable(SST::Statistics::StatisticInfo)
};



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
    
    // Enable/Disable of Statistic
    /** Enable Statistic for collections */
    void enable() {m_statEnabled = true;}
    
    /** Disable Statistic for collections */
    void disable() {m_statEnabled = false;}

    // Handling of Collection Counts and Data
    /** Inform the Statistic to clear its data */
    virtual void clearStatisticData() {}      

    /** Set the current collection count to 0 */
    virtual void resetCollectionCount();
    
    /** Increment current collection count */
    virtual void incrementCollectionCount();  
    
    /** Set the current collection count to a defined value */
    virtual void setCollectionCount(uint64_t newCount);
    
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
    
    // Get Data & Information on Statistic
    /** Return the Component Name */
    const std::string& getCompName() const;
    
    /** Return the Statistic Name */
    inline const std::string& getStatName() const {return m_statName;}
    
    /** Return the Statistic SubId */
    inline const std::string& getStatSubId() const {return m_statSubId;}
    
    /** Return the full Statistic name of Component.StatName.SubId */
    inline const std::string& getFullStatName() const {return m_statFullName;}      // Return compName.statName.subId
    
    /** Return the Statistic type name */
    inline const std::string& getStatTypeName() const {return m_statTypeName;}    
    
    /** Return the Statistic data type */
    inline const StatisticFieldInfo::fieldType_t& getStatDataType() const {return m_statDataType;}    
    
    /** Return the Statistic data type */
    inline const char* getStatDataTypeShortName() const {return StatisticFieldInfo::getFieldTypeShortName(m_statDataType);}
    
    /** Return the Statistic data type */
    inline const char* getStatDataTypeFullName() const {return StatisticFieldInfo::getFieldTypeFullName(m_statDataType);} 
    
    /** Return a pointer to the parent Component */
    BaseComponent*   getComponent() const {return m_component;}
    
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

    // Status of Statistic
    /** Indicate that the Statistic is Ready to be used */
    virtual bool isReady() const {return true;}     

    /** Indicate if the Statistic is a NullStatistic */
    virtual bool isNullStatistic() const {return false;} 

protected:
    friend class SST::Statistics::StatisticProcessingEngine;
    friend class SST::Statistics::StatisticOutput;
    friend class SST::Statistics::StatisticGroup;

    /** Construct a StatisticBase
      * @param comp - Pointer to the parent constructor.
      * @param statName - Name of the statistic to be registered.  This name must
      * match the name in the ElementInfoStatistic.
      * @param statSubId - Additional name of the statistic 
      * @param statParams - The parameters for this statistic
      */
    // Constructors:
    StatisticBase(BaseComponent* comp, const std::string& statName,
                  const std::string& statSubId, Params& statParams);

    // Destructor
    virtual ~StatisticBase() {}

    /** Return the Statistic Parameters */
    Params& getParams() {return m_statParams;}

    /** Set the Statistic Data Type */
    void setStatisticDataType(const StatisticFieldInfo::fieldType_t dataType) {m_statDataType = dataType;}

    /** Set an optional Statistic Type Name */
    void setStatisticTypeName(const char* typeName) {m_statTypeName = typeName;}
    
private:
    friend class SST::BaseComponent;

    /** Set the Registered Collection Mode */
    void setRegisteredCollectionMode(StatMode_t mode) {m_registeredCollectionMode = mode;} 

    /** Construct a full name of the statistic */
    static std::string buildStatisticFullName(const char* compName, const char* statName, const char* statSubId);
    static std::string buildStatisticFullName(const std::string& compName, const std::string& statName, const std::string& statSubId);

    // Required Virtual Methods:
    /** Called by the system to tell the Statistic to register its output fields.
      * by calling statOutput->registerField(...)
      * @param statOutput - Pointer to the statistic output
      */
    virtual void registerOutputFields(StatisticOutput* statOutput) = 0;  

    /** Called by the system to tell the Statistic to send its data to the 
      * StatisticOutput to be output.
      * @param statOutput - Pointer to the statistic output
      * @param EndOfSimFlag - Indicates that the output is occurring at the end of simulation.
      */
    virtual void outputStatisticData(StatisticOutput* statOutput, bool EndOfSimFlag) = 0;

    /** Indicate if the Statistic Mode is supported.
      * This allows Statistics to support STAT_MODE_COUNT and STAT_MODE_PERIODIC modes.
      * by default, both modes are supported.
      * @param mode - Mode to test
      */
    virtual bool isStatModeSupported(StatMode_t UNUSED(mode)) const {return true;}      // Default is to accept all modes

    /** Verify that the statistic names match */
    bool operator==(StatisticBase& checkStat); 

    // Support Routines
    void initializeStatName(const char* compName, const char* statName, const char* statSubId);
    void initializeStatName(const std::string& compName, const std::string& statName, const std::string& statSubId);

    void initializeProperties();
    void checkEventForOutput();

    // OneShot Callbacks:
    void delayOutputExpiredHandler();                               // Enable Output in handler
    void delayCollectionExpiredHandler();                           // Enable Collection in Handler

    const StatisticGroup* getGroup() const { return m_group; }
    void setGroup(const StatisticGroup *group ) { m_group = group; }

private:
    StatisticBase(); // For serialization only

private:
    BaseComponent*        m_component;
    std::string           m_statName;
    std::string           m_statSubId;
    std::string           m_statFullName;
    std::string           m_statTypeName;
    Params                m_statParams;
    StatMode_t            m_registeredCollectionMode;
    uint64_t              m_currentCollectionCount;
    uint64_t              m_collectionCountLimit;
    StatisticFieldInfo::fieldType_t m_statDataType;

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
    const StatisticGroup* m_group;

};

/**
 \class StatisticCollector
 * Base type that creates the virtual addData(...) interface
 * Used for distinguishing fundamental types (collected by value)
 * and composite struct types (collected by reference)
 */
template <class T, bool F=std::is_fundamental<T>::value>
struct StatisticCollector { };

template <class T>
struct StatisticCollector<T,true> {
 virtual void addData_impl(T data) = 0;
};

template <class T>
struct StatisticCollector<T,false> {
 virtual void addData_impl(T&& data) = 0;
 virtual void addData_impl(const T& data) = 0;
};

////////////////////////////////////////////////////////////////////////////////

/**
    \class Statistic

	Forms the template defined base class for statistics gathering within SST. 

	@tparam T A template for the basic numerical data stored by this Statistic
*/


template <typename T>
class Statistic : public StatisticBase, public StatisticCollector<T>
{
public:
    using StatisticCollector<T>::addData_impl;
    // The main method to add data to the statistic 
    /** Add data to the Statistic
      * This will call the addData_impl() routine in the derived Statistic.
     */
    template <class U> //use a universal reference here
    void addData(U&& data)
    {
        // Call the Derived Statistic's implementation 
        //  of addData and increment the count
        if (isEnabled()) {
            addData_impl(std::forward<U>(data));
            incrementCollectionCount();
        }
    }

    static FieldId_t fieldId() {
      return StatisticFieldType<T>::id();
    }

protected:
    friend class SST::Factory;
    friend class SST::BaseComponent;
    /** Construct a Statistic
      * @param comp - Pointer to the parent constructor.
      * @param statName - Name of the statistic to be registered.  This name must
      * match the name in the ElementInfoStatistic.
      * @param statSubId - Additional name of the statistic 
      * @param statParams - The parameters for this statistic
      */
    Statistic(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams) :
        StatisticBase(comp, statName, statSubId, statParams)
    {
        setStatisticDataType(StatisticFieldInfo::getFieldTypeFromTemplate<T>());
    }

    virtual ~Statistic(){}

private:     
    Statistic(){} // For serialization only

};

template <template <class> class Stat, class Field>
struct StatInstantiate {
  StatInstantiate(Params& p) : f(nullptr,"","",p) {}
  Stat<Field> f;
  bool isInstantiated() const {
    return f.isInstantiated();
  }
};


template <template <class> class Stat>
struct StatInstantiate<Stat, any_integer_type> :
  public StatInstantiate<Stat,int16_t>,
  public StatInstantiate<Stat,uint16_t>,
  public StatInstantiate<Stat,int32_t>,
  public StatInstantiate<Stat,uint32_t>,
  public StatInstantiate<Stat,int64_t>,
  public StatInstantiate<Stat,uint64_t>
{
  StatInstantiate(Params& p) :
    StatInstantiate<Stat,int16_t>(p),
    StatInstantiate<Stat,uint16_t>(p),
    StatInstantiate<Stat,int32_t>(p),
    StatInstantiate<Stat,uint32_t>(p),
    StatInstantiate<Stat,int64_t>(p),
    StatInstantiate<Stat,uint64_t>(p)
  {}

  bool isInstantiated() const {
    return StatInstantiate<Stat,int16_t>::isInstantiated();
  }
};

template <template <class> class Stat>
struct StatInstantiate<Stat, any_floating_type> :
  public StatInstantiate<Stat,float>,
  public StatInstantiate<Stat,double>
{
  StatInstantiate(Params& p) :
    StatInstantiate<Stat,float>(p),
    StatInstantiate<Stat,double>(p)
  {}

  bool isInstantiated() const {
    return StatInstantiate<Stat,double>::isInstantiated();
  }
};

template <template <class> class Stat>
struct StatInstantiate<Stat, any_numeric_type> :
  public StatInstantiate<Stat,any_integer_type>,
  public StatInstantiate<Stat,any_floating_type>
{
  StatInstantiate(Params& p) :
    StatInstantiate<Stat,any_integer_type>(p),
    StatInstantiate<Stat,any_floating_type>(p)
  {}

  bool isInstantiated() const {
    return StatInstantiate<Stat,any_integer_type>::isInstantiated();
  }
};

template <class T> class TemplateStatisticFieldIdInfo {};
template <template <class> class StatisticType, class Field>
struct TemplateStatisticFieldIdInfo<StatisticType<Field>> {
  static FieldId_t getId() { return StatisticFieldType<Field>::fieldId(); }
};

template <class T> struct TemplateStatInstantiation {};

template <template <class> class StatisticType, class FieldType>
struct TemplateStatInstantiation<StatisticType<FieldType>> {
  static bool isInstantiated() { return instantiated; }

  static bool instantiated;
};
template <template <class> class Stat, class T> bool
  TemplateStatInstantiation<Stat<T>>::instantiated = true;

class StatisticElementInfo : public BaseElementInfo {

public:
    virtual StatisticBase* create(BaseComponent* UNUSED(comp),
                                  const std::string& UNUSED(statName),
                                  const std::string& UNUSED(statSubId),
                                  Params& UNUSED(statParams)) {
      return nullptr;
    }

    virtual const std::string getInterface() = 0;

    std::string toString();

    virtual Statistics::FieldId_t fieldId() const = 0;
};

template <class T, unsigned V1, unsigned V2, unsigned V3>
class StatisticDoc : public StatisticElementInfo {
private:
    static const bool loaded;

public:

    StatisticDoc() : StatisticElementInfo() {
        //initialize_allowedKeys();
    }

    Statistics::FieldId_t fieldId() const override {
      return T::fieldId();
    }

    StatisticBase* create(BaseComponent* comp, const std::string& statName,
                          const std::string& statId, Params& params) override {
      return new T(comp, statName, statId, params);
    }

    static bool isLoaded() { return loaded; }
    const std::string getLibrary() override { return T::ELI_getLibrary(); }
    const std::string getName() override { return T::ELI_getName(); }
    const std::string getDescription() override { return T::ELI_getDescription(); }
    const std::vector<ElementInfoParam>& getValidParams() { return ELI_templatedGetParams<T>(); }
    const std::string getInterface() override { return T::ELI_getInterface(); }
    const std::vector<int>& getELICompiledVersion() override { return T::ELI_getELICompiledVersion(); }
    const std::vector<int>& getVersion() override { static std::vector<int> var = {V1,V2,V3}; return var; }
    const std::string getCompileFile() override { return T::ELI_getCompileFile(); }
    const std::string getCompileDate() override { return T::ELI_getCompileDate(); }
};
template<class T, unsigned V1, unsigned V2, unsigned V3> const bool StatisticDoc<T,V1,V2,V3>::loaded
  = ElementLibraryDatabase::addStatistic(new StatisticDoc<T,V1,V2,V3>());


#define SST_ELI_REGISTER_STATISTIC_TEMPLATE(cls,lib,name,version,desc,interface)   \
  bool ELI_isLoaded() { \
      return SST::Statistics::StatisticDoc<cls, \
        SST::SST_ELI_getMajorNumberFromVersion(version), \
        SST::SST_ELI_getMinorNumberFromVersion(version), \
        SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
  } \
  static const std::string ELI_getLibrary() { \
    return lib; \
  } \
  static const std::string ELI_getName() { \
    return name; \
  } \
  static const std::string ELI_getDescription() {  \
    return desc; \
  } \
  static const std::string ELI_getInterface() {  \
    return interface; \
  } \
  static const std::vector<int>& ELI_getVersion() { \
      static std::vector<int> var = version ; \
      return var; \
  } \
  static bool isInstantiated(){ \
    return SST::Statistics::TemplateStatInstantiation<cls>::isInstantiated(); \
  } \
  SST_ELI_INSERT_COMPILE_INFO()


#define SST_ELI_REGISTER_STATISTIC(cls,field,lib,name,version,desc,interface) \
  bool ELI_isLoaded() { \
      return SST::Statistics::StatisticDoc<cls,\
        SST::SST_ELI_getMajorNumberFromVersion(version), \
        SST::SST_ELI_getMinorNumberFromVersion(version), \
        SST::SST_ELI_getTertiaryNumberFromVersion(version)>::isLoaded(); \
  } \
  static const std::string ELI_getLibrary() { \
    return lib; \
  } \
  static const std::string ELI_getName() { \
    return name; \
  } \
  static const std::string ELI_getDescription() {  \
    return desc; \
  } \
  static const std::string ELI_getInterface() {  \
    return interface; \
  } \
  static const std::vector<int>& ELI_getVersion() { \
      static std::vector<int> var = version ; \
      return var; \
  } \
  SST_ELI_INSERT_COMPILE_INFO()

#define SST_ELI_INSTANTIATE_STATISTIC(cls,lib,allowedFields) \
  bool registerTemplateStat_##cls##_##lib(Params* p){ \
    SST::Statistics::StatInstantiate<cls,allowedFields> s(*p); \
    return s.isInstantiated(); \
  }


} //namespace Statistics
} //namespace SST

#endif
