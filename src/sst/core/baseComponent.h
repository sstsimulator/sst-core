// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_BASECOMPONENT_H
#define SST_CORE_BASECOMPONENT_H

#include <sst/core/sst_types.h>
#include <sst/core/warnmacros.h>

#include <map>
#include <string>

#include <sst/core/statapi/statengine.h>
#include <sst/core/statapi/statbase.h>
#include <sst/core/event.h>
#include <sst/core/clock.h>
#include <sst/core/oneshot.h>
#include <sst/core/componentInfo.h>
#include <sst/core/simulation.h>
#include <sst/core/eli/elementinfo.h>

using namespace SST::Statistics;

namespace SST {

class Clock;
class Link;
class LinkMap;
class Module;
class Params;
class SubComponent;
class TimeConverter;
class UnitAlgebra;
class SharedRegion;
class SharedRegionMerger;
class Component;
class ComponentExtension;
class SubComponent;
class SubComponentSlotInfo;

/**
 * Main component object for the simulation.
 */
class BaseComponent {

    friend class SubComponentSlotInfo;
    friend class SubComponent;
    friend class ComponentInfo;
    
public:

    BaseComponent(ComponentId_t id);
    BaseComponent() {}
    virtual ~BaseComponent();

    /** Returns a pointer to the parent BaseComponent */
    BaseComponent* getParent() const __attribute__ ((deprecated("getParent() will be removed in SST version 10.0.  With the new subcomponent structure, direct access to your parent is not allowed.")))
        { return my_info->parent_info->component; }

    const std::string& getType() const { return my_info->getType(); }
    
    /** Returns unique component ID */
    inline ComponentId_t getId() const { return my_info->id; }

    /** Called when SIGINT or SIGTERM has been seen.
     * Allows components opportunity to clean up external state.
     */
    virtual void emergencyShutdown(void) {}


	/** Returns component Name */
	/* inline const std::string& getName() const { return name; } */
	inline const std::string& getName() const { return my_info->getName(); }


    /** Used during the init phase.  The method will be called each phase of initialization.
     Initialization ends when no components have sent any data. */
    virtual void init(unsigned int UNUSED(phase)) {}
    /** Used during the init phase.  The method will be called each phase of initialization.
     Initialization ends when no components have sent any data. */
    virtual void complete(unsigned int UNUSED(phase)) {}
    /** Called after all components have been constructed and initialization has
	completed, but before simulation time has begun. */
    virtual void setup( ) { }
    /** Called after simulation completes, but before objects are
        destroyed. A good place to print out statistics. */
    virtual void finish( ) { }

    /** Currently unused function */
    virtual bool Status( ) { return 0; }

    /**
     * Called by the Simulation to request that the component
     * print it's current status.  Useful for debugging.
     * @param out The Output class which should be used to print component status.
     */
    virtual void printStatus(Output &UNUSED(out)) { return; }

    /** Determine if a port name is connected to any links */
    bool isPortConnected(const std::string &name) const;

    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link.  If NULL is passed in, then it
     * will use the Component defaultTimeBase
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param time_base - Time Base of the link as a string
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    /** Configure a Link
     * @param name - Port Name on which the link to configure is attached.
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureLink( std::string name, Event::HandlerBase* handler = NULL);

    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link.  If NULL is passed in, then it
     * will use the Component defaultTimeBase
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, TimeConverter* time_base, Event::HandlerBase* handler = NULL);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param time_base - Time Base of the link
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, std::string time_base, Event::HandlerBase* handler = NULL);
    /** Configure a SelfLink  (Loopback link)
     * @param name - Name of the self-link port
     * @param handler - Optional Handler to be called when an Event is received
     * @return A pointer to the configured link, or NULL if an error occured.
     */
    Link* configureSelfLink( std::string name, Event::HandlerBase* handler = NULL);

    /** Registers a clock for this component.
        @param freq Frequency for the clock in SI units
        @param handler Pointer to Clock::HandlerBase which is to be invoked
        at the specified interval
        @param regAll Should this clock period be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerClock( std::string freq, Clock::HandlerBase* handler,
                                  bool regAll = true);
    TimeConverter* registerClock( const UnitAlgebra& freq, Clock::HandlerBase* handler,
                                  bool regAll = true);
    /** Removes a clock handler from the component */
    void unregisterClock(TimeConverter *tc, Clock::HandlerBase* handler);

    /** Reactivates an existing Clock and Handler
     * @return time of next time clock handler will fire
     */
    Cycle_t reregisterClock(TimeConverter *freq, Clock::HandlerBase* handler);
    /** Returns the next Cycle that the TimeConverter would fire */
    Cycle_t getNextClockCycle(TimeConverter *freq);

    /** Registers a OneShot event for this component.
        Note: OneShot cannot be canceled, and will always callback after
          the timedelay.
        @param timeDelay Time delay for the OneShot in SI units
        @param handler Pointer to OneShot::HandlerBase which is to be invoked
        at the specified interval
    */
    TimeConverter* registerOneShot( std::string timeDelay, OneShot::HandlerBase* handler);
    TimeConverter* registerOneShot( const UnitAlgebra& timeDelay, OneShot::HandlerBase* handler);

    /** Registers a default time base for the component and optionally
        sets the the component's links to that timebase. Useful for
        components which do not have a clock, but would like a default
        timebase.
        @param base Frequency for the clock in SI units
        @param regAll Should this clock period be used as the default
        time base for all of the links connected to this component
    */
    TimeConverter* registerTimeBase( std::string base, bool regAll = true);

    TimeConverter* getTimeConverter( const std::string& base );
    TimeConverter* getTimeConverter( const UnitAlgebra& base );

    /** return the time since the simulation began in units specified by
        the parameter.
        @param tc TimeConverter specifying the units */
    SimTime_t getCurrentSimTime(TimeConverter *tc) const;
    /** return the time since the simulation began in the default timebase */
    inline SimTime_t getCurrentSimTime()  const{
        return getCurrentSimTime(my_info->defaultTimeBase);
    }
    /** return the time since the simulation began in timebase specified
        @param base Timebase frequency in SI Units */
    SimTime_t getCurrentSimTime(std::string base);

    /** Utility function to return the time since the simulation began in nanoseconds */
    SimTime_t getCurrentSimTimeNano() const;
    /** Utility function to return the time since the simulation began in microseconds */
    SimTime_t getCurrentSimTimeMicro() const;
    /** Utility function to return the time since the simulation began in milliseconds */
    SimTime_t getCurrentSimTimeMilli() const;

    bool isStatisticShared(const std::string& statName, bool include_me = false) {
        if ( include_me ) {
            if ( doesComponentInfoStatisticExist(statName)) {
                return true;
            }
        }
        if ( my_info->sharesStatistics() ) {
            return my_info->parent_info->component->isStatisticShared(statName, true);
        }
        else {
            return false;
        }
    }


    /** Registers a statistic.
        If Statistic is allowed to run (controlled by Python runtime parameters),
        then a statistic will be created and returned. If not allowed to run,
        then a NullStatistic will be returned.  In either case, the returned
        value should be used for all future Statistic calls.  The type of
        Statistic and the Collection Rate is set by Python runtime parameters.
        If no type is defined, then an Accumulator Statistic will be provided
        by default.  If rate set to 0 or not provided, then the statistic will
        output results only at end of sim (if output is enabled).
        @param statName Primary name of the statistic.  This name must match the
               defined ElementInfoStatistic in the component, and must also
               be enabled in the Python input file.
        @param statSubId An additional sub name for the statistic
        @return Either a created statistic of desired type or a NullStatistic
                depending upon runtime settings.
    */
    template <typename T>
    Statistic<T>* registerStatistic(SST::Params& params, const std::string& statName, const std::string& statSubId = "")
    {
        // Verify here that name of the stat is one of the registered
        // names of the component's ElementInfoStatistic.
        if (false == doesComponentInfoStatisticExist(statName)) {
            // I don't define this statistic, so it must be inherited (or non-existant)
            if ( my_info->sharesStatistics() ) {
                return my_info->parent_info->component->registerStatistic<T>(params, statName, statSubId);
            }
            else {
                printf("Error: Statistic %s name %s is not found in ElementInfoStatistic, exiting...\n",
                       StatisticBase::buildStatisticFullName(getName().c_str(), statName, statSubId).c_str(),
                       statName.c_str());
                exit(1);
            }
        }
        // Check to see if the Statistic is previously registered with the Statistics Engine
        auto* engine = StatisticProcessingEngine::getInstance();
        StatisticBase* stat = engine->isStatisticRegisteredWithEngine(getName(), my_info->getID(), statName, statSubId, StatisticFieldType<T>::id());

        if (!stat){
            //stat does not exist yet
            auto makeInstance = [=](const std::string& type, BaseComponent* comp,
                                    const std::string& name, const std::string& subName,
                                    SST::Params& params) -> StatisticBase* {
                return engine->createStatistic<T>(comp, type, name, subName, params);
            };
            stat = registerStatisticCore(params, statName, statSubId, StatisticFieldType<T>::id(),
                                         std::move(makeInstance));
        }
        return dynamic_cast<Statistic<T>*>(stat);
    }

    template <typename T>
    Statistic<T>* registerStatistic(const std::string& statName, const std::string& statSubId = "")
    {
        SST::Params empty{};
        return registerStatistic<T>(empty, statName, statSubId);
    }

    template <typename... Args>
    Statistic<std::tuple<Args...>>* registerMultiStatistic(const std::string& statName, const std::string& statSubId = "")
    {
        SST::Params empty{};
        return registerStatistic<std::tuple<Args...>>(empty, statName, statSubId);
    }

    template <typename... Args>
    Statistic<std::tuple<Args...>>* registerMultiStatistic(SST::Params& params, const std::string& statName,
                                                           const std::string& statSubId = "")
    {
        return registerStatistic<std::tuple<Args...>>(params, statName, statSubId);
    }

    template <typename T>
    Statistic<T>* registerStatistic(const char* statName, const char* statSubId = "")
    {
        return registerStatistic<T>(std::string(statName), std::string(statSubId));
    }


    /** Loads a module from an element Library
     * @param type Fully Qualified library.moduleName
     * @param params Parameters the module should use for configuration
     * @return handle to new instance of module, or NULL on failure.
     */
    Module* loadModule(std::string type, Params& params);

    /** Loads a module from an element Library
     * @param type Fully Qualified library.moduleName
     * @param comp Pointer to component to pass to Module's constructor
     * @param params Parameters the module should use for configuration
     * @return handle to new instance of module, or NULL on failure.
     */
    Module* loadModuleWithComponent(std::string type, Component* comp, Params& params) __attribute__ ((deprecated("loadModuleWithComponent will be removed in SST version 10.0.  If the module needs access to the parent component, please use SubComponents instead of Modules.")));


    /** Loads a SubComponent from an element Library
     * @param type Fully Qualified library.moduleName
     * @param comp Pointer to component to pass to SuBaseComponent's constructor
     * @param params Parameters the module should use for configuration
     * @return handle to new instance of SubComponent, or NULL on failure.
     */
    SubComponent* loadSubComponent(std::string type, Component* comp, Params& params) __attribute__ ((deprecated("This version of loadSubComponent will be removed in SST version 10.0.  Please switch to new user defined API (LoadUserSubComponent(std::string, int, ARGS...)).")));

    /* New ELI style */
    SubComponent* loadNamedSubComponent(std::string name) __attribute__ ((deprecated("This version of loadNamedSubComponent will be removed in SST version 10.0.  Please switch to new user defined API (LoadUserSubComponent(std::string, int, ARGS...)).")));
    SubComponent* loadNamedSubComponent(std::string name, Params& params) __attribute__ ((deprecated("This version of loadNamedSubComponent will be removed in SST version 10.0.  Please switch to new user defined API (LoadUserSubComponent(std::string, int, ARGS...)).")));

    
protected:
    // When you direct load, the ComponentExtension does not need any
    // ELI information and if it has any, it will be ignored.  The
    // extension will be loaded as if it were part of the part
    // BaseComponent and will share all that components ELI
    // information.
    template <class T, class... ARGS>
    T* loadComponentExtension(ARGS... args) {
        ComponentExtension* ret = new T(my_info->id, args...);
        return static_cast<T*>(ret);
    }

    /**
       Check to see if a given element type is loadable with a particular API
       @param name - Name of element to check in lib.name format
       @return True if loadable as the API specified as the template parameter
     */
    template <class T>
    bool isSubComponentLoadableUsingAPI(std::string type) {
        return Factory::getFactory()->isSubComponentLoadableUsingAPI<T>(type);
    }
    
    /**
       Loads an anonymous subcomponent (not defined in input file to
       SST run).

       @param type tyupe of subcomponent to load in lib.name format
       @param slot_name name of the slot to load subcomponent into
       @param slot_num  index of the slot to load subcomponent into
       @param share_flags Share flags to be used by subcomponent
       @param params Params object to be passed to subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T, class... ARGS>
    T* loadAnonymousSubComponent(std::string type, std::string slot_name, int slot_num, uint64_t share_flags, Params& params, ARGS... args) {

        share_flags = share_flags & ComponentInfo::USER_FLAGS;
        ComponentId_t cid = my_info->addAnonymousSubComponent(my_info, type, slot_name, slot_num, share_flags);
        ComponentInfo* sub_info = my_info->findSubComponent(cid);

        //This shouldn't happen since we just put it in, but just in case
        if ( sub_info == NULL ) return NULL;

        // Check to see if this can be loaded with new API or if we have to fallback to old
        if ( isSubComponentLoadableUsingAPI<T>(type) ) {
            auto ret = Factory::getFactory()->Create<T>(type, params, sub_info->id, params, args...);
            return ret;
        }
        else {
            SubComponent* ret = loadLegacySubComponentPrivate(cid,type,params);
            return dynamic_cast<T*>(ret);
        }
    }


    /**
       Loads a user defined subcomponent (defined in input file to SST
       run).  This version does not allow share flags (set to
       SHARE_NONE) or constructor arguments.

       @param slot_name name of the slot to load subcomponent into

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T>
    T* loadUserSubComponent(std::string slot_name) {
        return loadUserSubComponent<T>(slot_name, ComponentInfo::SHARE_NONE);
    }
    
    /**
       Loads a user defined subcomponent (defined in input file to SST
       run).

       @param slot_name name of the slot to load subcomponent into
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T, class... ARGS>
    T* loadUserSubComponent(std::string slot_name, uint64_t share_flags, ARGS... args) {

        // Get list of ComponentInfo objects and make sure that there is
        // only one SubComponent put into this slot
        // const std::vector<ComponentInfo>& subcomps = my_info->getSubComponents();
        const std::map<ComponentId_t,ComponentInfo>& subcomps = my_info->getSubComponents();
        int sub_count = 0;
        int index = -1;
        for ( auto &ci : subcomps ) {
            if ( ci.second.getSlotName() == slot_name ) {
                index = ci.second.getSlotNum();
                sub_count++;
            }
        }
        
        if ( sub_count > 1 ) {
            SST::Output outXX("SubComponentSlotWarning: ", 0, 0, Output::STDERR);
            outXX.fatal(CALL_INFO, 1, "Error: ComponentSlot \"%s\" in component \"%s\" only allows for one SubComponent, %d provided.\n",
                        slot_name.c_str(), my_info->getType().c_str(), sub_count);
        }
        
        return loadUserSubComponentByIndex<T,ARGS...>(slot_name, index, share_flags, args...);        
    }
    
private:

    SubComponent* loadNamedSubComponent(std::string name, int slot_num);
    SubComponent* loadNamedSubComponent(std::string name, int slot_num, Params& params);

    SubComponent* loadNamedSubComponentLegacyPrivate(ComponentInfo* sub_info, Params& params);

    
    SubComponent* loadLegacySubComponentPrivate(ComponentId_t cid, const std::string& type, Params& params);


    // These two functions are only need for backward compatibility
    // for anonymous subcomponents.
    void setDefaultTimeBaseForParentLinks(TimeConverter* tc);
    void setDefaultTimeBaseForChildLinks(TimeConverter* tc);

    void setDefaultTimeBaseForLinks(TimeConverter* tc);

    void pushValidParams(Params& params, const std::string& type);
    
    template <class T, class... ARGS>
    T* loadUserSubComponentByIndex(std::string slot_name, int slot_num, int share_flags, ARGS... args) {

        share_flags = share_flags & ComponentInfo::USER_FLAGS;
        
        // Check to see if the slot exists
        ComponentInfo* sub_info = my_info->findSubComponent(slot_name,slot_num);
        if ( sub_info == NULL ) return NULL;
        sub_info->share_flags = share_flags;
        sub_info->parent_info = my_info;
        
        // Check to see if this is documented, and if so, try to load it through the ElementBuilder
        Params myParams;
        if ( sub_info->getParams() != NULL ) {
            myParams.insert(*sub_info->getParams());
        }

        if ( isSubComponentLoadableUsingAPI<T>(sub_info->type) ) {
            auto ret = Factory::getFactory()->Create<T>(sub_info->type, myParams, sub_info->id, myParams, args...);
            return ret;
        }
        else {
            SubComponent* ret = loadNamedSubComponentLegacyPrivate(sub_info,myParams);
            return dynamic_cast<T*>(ret);
        }
        
        // return nullptr;        
    }

    ComponentInfo* getCurrentlyLoadingSubComponentInfo() { return currentlyLoadingSubComponent; }
    ComponentId_t getCurrentlyLoadingSubComponentID() { return currentlyLoadingSubComponentID; }


public:
    SubComponentSlotInfo* getSubComponentSlotInfo(std::string name, bool fatalOnEmptyIndex = false);

    /** Retrieve the X,Y,Z coordinates of this component */
    const std::vector<double>& getCoordinates() const {
        return my_info->coordinates;
    }

protected:
    friend class SST::Statistics::StatisticProcessingEngine;

    bool isAnonymous() {
        return my_info->isAnonymous();
    }

    bool isUser() {
        return my_info->isUser();
    }
    
    /** Manually set the default detaulTimeBase */
    void setDefaultTimeBase(TimeConverter *tc) {
        my_info->defaultTimeBase = tc;
    }

    TimeConverter* getDefaultTimeBase() {
        return my_info->defaultTimeBase;
    }

    bool doesSubComponentExist(std::string type);


    /** Find a lookup table */
    SharedRegion* getLocalSharedRegion(const std::string &key, size_t size);
    SharedRegion* getGlobalSharedRegion(const std::string &key, size_t size, SharedRegionMerger *merger = NULL);

    /* Get the Simulation */
    Simulation* getSimulation() const { return sim; }

    // Does the statisticName exist in the ElementInfoStatistic
    virtual bool doesComponentInfoStatisticExist(const std::string &statisticName) const;
    // Return the EnableLevel for the statisticName from the ElementInfoStatistic
    uint8_t getComponentInfoStatisticEnableLevel(const std::string &statisticName) const;
    // Return the Units for the statisticName from the ElementInfoStatistic
    // std::string getComponentInfoStatisticUnits(const std::string &statisticName) const;

    
    Component* getTrueComponent() const __attribute__ ((deprecated("getTrueParent will be removed in SST version 10.0.  With the new subcomponent structure, direct access to your parent component is not allowed.")));


protected:
    Simulation *sim;


private:

    // Only need temporarily to help with backward compatibility
    // implementation in elements.
    bool loadedWithLegacyAPI;

public:

    /**
       Temporary function to help provide backward compatibility to
       old SubComponent API.
       
       @return true if subcomponent loaded with old API, false if
       loaded with new
     */
    bool wasLoadedWithLegacyAPI() const {
        return loadedWithLegacyAPI;
    }
    
private:

    ComponentInfo* my_info;
    ComponentInfo* currentlyLoadingSubComponent;
    ComponentId_t currentlyLoadingSubComponentID;

    void addSelfLink(std::string name);
    Link* getLinkFromParentSharedPort(const std::string& port);

    using CreateFxn = std::function<StatisticBase*(const std::string&,
                            BaseComponent*,const std::string&, const std::string&, SST::Params&)>;

    StatisticBase* registerStatisticCore(SST::Params& params,
                                         const std::string& statName, const std::string& statSubId,
                                         fieldType_t fieldType, CreateFxn&& fxn);

    Component* getTrueComponentPrivate() const;

};


/**
   Used to load SubComponents when multiple SubComponents are loaded
   into a single slot (will also also work when a single SubComponent
   is loaded).
 */
class SubComponentSlotInfo {

    BaseComponent* comp;
    std::string slot_name;
    int max_slot_index;

protected:
    
    SubComponent* protected_create(int slot_num, Params& params) const {
        if ( slot_num > max_slot_index ) return NULL;

        return comp->loadNamedSubComponent(slot_name, slot_num, params);
    }    



    
public:
    ~SubComponentSlotInfo() {}
    

    SubComponentSlotInfo(BaseComponent* comp, std::string slot_name) :
        comp(comp),
        slot_name(slot_name)
    {
        const std::map<ComponentId_t,ComponentInfo>& subcomps = comp->my_info->getSubComponents();

        // Look for all subcomponents with the right slot name
        max_slot_index = -1;
        for ( auto &ci : subcomps ) {
            if ( ci.second.getSlotName() == slot_name ) {
                if ( ci.second.getSlotNum() > static_cast<int>(max_slot_index) ) {
                    max_slot_index = ci.second.getSlotNum();
                }
            }
        }
    }

    const std::string& getSlotName() const {
        return slot_name;
    };

    bool isPopulated(int slot_num) const {
        if ( slot_num > max_slot_index ) return false;
        if ( comp->my_info->findSubComponent(slot_name,slot_num) == NULL ) return false;
        return true;
    }

    bool isAllPopulated() const {
        for ( int i = 0; i < max_slot_index; ++i ) {
            if ( comp->my_info->findSubComponent(slot_name,i) == NULL ) return false;
        }
        return true;
    }

    int getMaxPopulatedSlotNumber() const {
        return max_slot_index;
    }


    // Create functions that support the legacy API

    template <typename T>
    __attribute__ ((deprecated("This version of create will be removed in SST version 10.0.  Please switch to the new user defined API, which includes the share flags.")))
    T* create(int slot_num, Params& params) const 
    {
        return private_create<T>(slot_num,params);
    }

    template <typename T>
    __attribute__ ((deprecated("This version of createAll will be removed in SST version 10.0.  Please switch to the new user defined API, which includes the share flags and optional constructor arguments.")))
    void createAll(Params& params, std::vector<T*>& vec, bool insertNulls = true) const 
    {
        return private_createAll<T>(params, vec, insertNulls);
    }

    template <typename T>
    __attribute__ ((deprecated("This version of createAll will be removed in SST version 10.0.  Please switch to the new user defined API, which includes the share flags and optional constructor arguments.")))
    void createAll(std::vector<T*>& vec, bool insertNulls = true) const 
    {
        Params empty;
        return private_createAll<T>(empty, vec, insertNulls);
    }


    // Create functions that support the new API

    /**
       Create a user defined subcomponent (defined in input file to
       SST run).  This call will pass SHARE_NONE to the new
       subcomponent and will not take constructor arguments.  If
       constructor arguments are needed for the API that is being
       loaded, the full call to create will need to be used
       create(slot_num, share_flags, args...).

       @param slot_num Slot index from which to load subcomponent

       This function supports the new API, but is identical to an
       existing API call.  It will try to load using new API and will
       fallback to old if unsuccessful.
    */
    template <typename T>
    T* create(int slot_num) const 
    {
        Params empty;
        return comp->loadUserSubComponentByIndex<T>(slot_name, slot_num, ComponentInfo::SHARE_NONE);
        // return private_create<T>(slot_num, empty);
    }

    
    /**
       Create a user defined subcomponent (defined in input file to SST
       run).

       @param slot_num Slot index from which to load subcomponent
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <class T, class... ARGS>
    T* create(int slot_num, uint64_t share_flags, ARGS... args) const {
        return comp->loadUserSubComponentByIndex<T,ARGS...>(slot_name,slot_num, share_flags, args...);
    }

    
    /**
       Create all user defined subcomponents (defined in input file to SST
       run) for the slot.

       @param vec Vector of T* that will hold the pointers to the new
       subcomponents.  If an index is not occupied, a nullptr will be
       put in it's place.  All components will be added to the end of
       the vector, so index N will be at vec.length() + N, where
       vec.length() is the length of the vector when it is passed to
       the call.
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <typename T, class... ARGS>
    void createAll(std::vector<T*>& vec, uint64_t share_flags, ARGS... args) const {
        for ( int i = 0; i <= getMaxPopulatedSlotNumber(); ++i ) {
            T* sub = create<T>(i, share_flags, args...);
            vec.push_back(sub);
        }
    }

    /**
       Create all user defined subcomponents (defined in input file to SST
       run) for the slot.

       @param vec Vector of pair<int,T*> that will hold the pointers
       to the new subcomponents.  The int will hold the index from
       which the subcomponent wass loaded.  Unoccupied indexes will be
       skipped.  All components will be added to the end of the
       vector.
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <typename T, class... ARGS>
    void createAllSparse(std::vector<std::pair<int,T*> >& vec, uint64_t share_flags, ARGS... args) const {
        for ( int i = 0; i <= getMaxPopulatedSlotNumber(); ++i ) {
            T* sub = create<T>(i, share_flags, args...);
            vec.push_back(i,sub);
        }
    }

    /**
       Create all user defined subcomponents (defined in input file to SST
       run) for the slot.

       @param vec Vector of T* that will hold the pointers
       to the new subcomponents.  Unoccupied indexes will be
       skipped.  All components will be added to the end of the
       vector.
       @param share_flags Share flags to be used by subcomponent
       @param args Arguments to be passed to constructor.  This
       signature is defined in the API definition

       For ease in backward compatibility to old API, this call will
       try to load using new API and will fallback to old if
       unsuccessful.
    */
    template <typename T, class... ARGS>
    void createAllSparse(std::vector<T*>& vec, uint64_t share_flags, ARGS... args) const {
        for ( int i = 0; i <= getMaxPopulatedSlotNumber(); ++i ) {
            T* sub = create<T>(i, share_flags, args...);
            vec.push_back(sub);
        }
    }

private:

    // Extra versions of the calls supporting the legacy API to avoid
    // deprecation warnings in every file that include this file
    template <typename T>
    T* private_create(int slot_num, Params& params) const
    {
        SubComponent* sub = protected_create(slot_num, params);
        if ( sub == NULL ) {
            // Nothing populated at this index, simply return NULL
            return NULL;
        }
        T* cast_sub = dynamic_cast<T*>(sub);
        if ( cast_sub == NULL ) {
            // SubComponent not castable to the correct class,
            // fatal
            Simulation::getSimulationOutput().fatal(CALL_INFO,1,"Attempt to load SubComponent into slot "
                                                    "%s, index %d, which is not castable to correct time\n",
                                                    getSlotName().c_str(),slot_num);
        }
        return cast_sub;
    }

    template <typename T>
    void private_createAll(Params& params, std::vector<T*>& vec, bool insertNulls = true) const 
    {
        for ( int i = 0; i <= getMaxPopulatedSlotNumber(); ++i ) {
            T* sub = create<T>(i, params);
            if ( sub != NULL || insertNulls ) vec.push_back(sub);
        }
    }

};



} //namespace SST

#endif // SST_CORE_BASECOMPONENT_H
