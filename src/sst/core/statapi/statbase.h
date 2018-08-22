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
    StatisticBase(BaseComponent* comp, const std::string& statName, const std::string& statSubId, Params& statParams);

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

////////////////////////////////////////////////////////////////////////////////

/**
    \class Statistic

	Forms the template defined base class for statistics gathering within SST. 

	@tparam T A template for the basic numerical data stored by this Statistic
*/

template <typename T>
class Statistic : public StatisticBase
{
public:
    // The main method to add data to the statistic 
    /** Add data to the Statistic
      * This will call the addData_impl() routine in the derived Statistic.
     */
    void addData(T data)
    {
        // Call the Derived Statistic's implementation 
        //  of addData and increment the count
        if (true == isEnabled()) {
            addData_impl(data);
            incrementCollectionCount();
        }
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
    Statistic(){}; // For serialization only

    // Required Templated Virtual Methods:
    virtual void addData_impl(T data) = 0;

private:
};

class StatisticFactory {
 public:
  virtual StatisticBase* build(BaseComponent* comp, const std::string& type,
                               const std::string& statName,
                               const std::string& statId, Params& params) = 0;

  static StatisticBase* create(FieldId_t id, BaseComponent* comp,
                       const std::string& type, const std::string& statName,
                       const std::string& statId, Params& params);

  template <class FieldType> static FieldId_t registerField();

 private:
  static std::map<FieldId_t,StatisticFactory*>* factories_;

};

template <class FieldType>
class StatisticBuilderBase {
 public:
  virtual Statistic<FieldType>* build(BaseComponent* comp, const std::string& statName,
                               const std::string& statId, Params& params) = 0;
};

template <class FieldType>
class StatisticFieldFactory : public StatisticFactory {
 public:
  static Statistic<FieldType>* create(BaseComponent* comp, const std::string& type,
                                      const std::string& statName,
                                      const std::string& statId, Params& params){
    if (!infos_){
      infos_ = new std::map<std::string, StatisticBuilderBase<FieldType>*>;
    }
    auto iter = infos_->find(type);
    if (iter == infos_->end()){
      Output::getDefaultObject().fatal(CALL_INFO,
        1, "Invalid statistic object %s requested", statName.c_str());
    }
    StatisticBuilderBase<FieldType>* info = iter->second;
    return info->build(comp, statName, statId, params);
  }

  StatisticBase* build(BaseComponent *comp, const std::string& type,
                       const std::string &statName,
                       const std::string &statId, Params &params) override {
    return create(comp, type, statName, statId, params);
  }

  static FieldId_t fieldId(){
    return StatisticFieldType<FieldType>::fieldId();
  }

  template <template <class> class StatCollector>
  static void registerTemplateBuilder(const std::string& name);

  template <class StatCollector>
  static void registerBuilder(const std::string& name);

 private:
  static std::map<std::string, StatisticBuilderBase<FieldType>*>* infos_;
};
template <class FieldType> std::map<std::string, StatisticBuilderBase<FieldType>*>*
  StatisticFieldFactory<FieldType>::infos_ = nullptr;


template <class FieldType>
FieldId_t StatisticFactory::registerField(){
  FieldId_t id = StatisticFieldType<FieldType>::id();
  if (!factories_) factories_ = new std::map<FieldId_t, StatisticFactory*>;

  auto iter = factories_->find(id);
  if (iter == factories_->end()){
    (*factories_)[id] = new StatisticFieldFactory<FieldType>;
  }

  return id;
}

template <class T, class FieldType>
class StatisticBuilder : public StatisticBuilderBase<FieldType> {
 public:
  Statistic<FieldType>* build(BaseComponent* comp, const std::string& statName,
                              const std::string& statId, Params& params) override {
    return new T(comp, statName, statId, params);
  }
};

template <class FieldType>
template <template <class> class StatCollector>
void StatisticFieldFactory<FieldType>::registerTemplateBuilder(const std::string& name){
  if (!infos_){
    infos_ = new std::map<std::string, StatisticBuilderBase<FieldType>*>;
  }
  auto iter = infos_->find(name);
  if (iter == infos_->end()){
    //these can get added redundantly
    (*infos_)[name] = new StatisticBuilder<StatCollector<FieldType>, FieldType>;
  }
  StatisticFactory::registerField<FieldType>();
}

template <class FieldType>
template <class StatCollector>
void StatisticFieldFactory<FieldType>::registerBuilder(const std::string& name){
  if (!infos_){
    infos_ = new std::map<std::string, StatisticBuilderBase<FieldType>*>;
  }
  auto iter = infos_->find(name);
  if (iter == infos_->end()){
    //these can get added redundantly
    infos_[name] = new StatisticBuilder<StatCollector, FieldType>;
  }
  StatisticFactory::registerField<FieldType>();
}


template <template <class> class StatisticType, class FieldType>
struct TemplateStatsRegistration {
  static void registerBuilder(const std::string& name){
    StatisticFieldFactory<FieldType>::template registerTemplateBuilder<StatisticType>(name);
  }
};

template <class T> struct TemplateStatBuilderRegistration {};

template <template <class> class StatisticType, class FieldType>
struct TemplateStatBuilderRegistration<StatisticType<FieldType>> {
  TemplateStatBuilderRegistration(){
    const char* name = StatisticType<FieldType>::factoryName();
    TemplateStatsRegistration<StatisticType,FieldType>::registerBuilder(name);
    instantiated = true;
  }

  bool isRegistered() const { return instantiated; }

  bool instantiated;
};

template <class T> struct MakeTemplateStatRegistered {
  static TemplateStatBuilderRegistration<T> instantiater;
  static bool isRegistered(){ return instantiater.isRegistered(); }
};
template <class T> TemplateStatBuilderRegistration<T> MakeTemplateStatRegistered<T>::instantiater;

template <class StatisticType, class FieldType>
struct StatsRegistration {
  static void registerBuilder(const std::string& name){
    StatisticFieldFactory<FieldType>::template registerBuilder<StatisticType>(name);
  }
};

template <class StatisticType, class FieldType>
struct StatBuilderRegistration {
  StatBuilderRegistration(){
    const char* name = StatisticType::factoryName();
    StatsRegistration<StatisticType,FieldType>::registerBuilder(name);
  }

  constexpr bool isRegistered() const { return true; }
};

template <class T, class Field>
bool isStatRegistered() {
  static StatBuilderRegistration<T,Field> builder;
  return builder.isRegistered();
}

template <template <class> class Stat, class Field>
struct StatInstantiate {
  StatInstantiate(Params& p) : f(nullptr,"","",p) {}
  Stat<Field> f;
  bool isRegistered() const {
    return f.isRegistered();
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

  bool isRegistered() const {
    return StatInstantiate<Stat,int16_t>::isRegistered();
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

  bool isRegistered() const {
    return StatInstantiate<Stat,double>::isRegistered();
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

  bool isRegistered() const {
    return StatInstantiate<Stat,any_integer_type>::isRegistered();
  }
};

template <template <class> class Stat, class Field>
bool registerTemplateStatType(Params* p){
  StatInstantiate<Stat, Field> s(*p);
  return s.isRegistered();
}

template <class T> class FieldIdGetter {};
template <template <class> class StatisticType, class Field>
struct FieldIdGetter<StatisticType<Field>> {
  static FieldId_t getId() { return StatisticFieldType<Field>::fieldId(); }
};

template <class T> //T is a full type Statistic<T>
FieldId_t getStatFieldId() {
  return FieldIdGetter<T>::getId();
}

#define SST_ELI_REGISTER_STATISTIC_TEMPLATE(cls)   \
  static Statistics::FieldId_t fieldId(){ return getStatFieldId<cls>(); } \
  static const char* factoryName(){ return #cls; } \
  static bool isRegistered(){ \
    return MakeTemplateStatRegistered<cls>::isRegistered(); \
  }

#define SST_ELI_REGISTER_STATISTIC(cls,field,fieldName,shortName) \
  static Statistics::FieldId_t fieldId(){ return StatisticFactoryBase<field>::fieldId(); } \
  static const char* factoryName(){ return #cls; } \
  static const char* fieldName(){ return fieldName; } \
  static const char* fieldShortName(){ return fieldShortName; } \
  static bool isRegistered(){ \
    return isStatRegistered<cls>(); \
  }

#define SST_ELI_INSTANTIATE_STATISTIC(cls,allowedFields,unique_id) \
  bool registerTemplateStat_##cls##_##unique_id(Params* p){ \
    SST::Statistics::StatInstantiate<cls,allowedFields> s(*p); \
    return s.isRegistered(); \
  }


} //namespace Statistics
} //namespace SST

#endif
