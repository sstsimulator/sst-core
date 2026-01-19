// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/model/configGraph.h"

#include "sst/core/component.h"
#include "sst/core/config.h"
#include "sst/core/factory.h"
#include "sst/core/from_string.h"
#include "sst/core/model/configLink.h"
#include "sst/core/model/configStatistic.h"
#include "sst/core/namecheck.h"
#include "sst/core/simulation_impl.h"
#include "sst/core/timeLord.h"
#include "sst/core/warnmacros.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>
#include <utility>

namespace {

// Functions to check component and link names
int bad_comp_name_count = 0;
int bad_link_name_count = 0;

const int max_invalid_name_prints = 10;

void
checkForValidComponentName(const std::string& name)
{
    if ( SST::NameCheck::isComponentNameValid(name) ) return;
    if ( bad_comp_name_count < max_invalid_name_prints ) {
        printf("WARNING: Component name '%s' is not valid\n", name.c_str());
        bad_comp_name_count++;
    }
    else if ( bad_comp_name_count == max_invalid_name_prints ) {
        printf("WARNING: Number of invalid component names exceeds limit of %d, no more messages will be printed\n",
            max_invalid_name_prints);
        bad_comp_name_count++;
    }
}

void
checkForValidLinkName(const std::string& name)
{
    if ( SST::NameCheck::isLinkNameValid(name) ) return;
    if ( bad_link_name_count < max_invalid_name_prints ) {
        printf("WARNING: Link name '%s' is not valid\n", name.c_str());
        bad_link_name_count++;
    }
    else if ( bad_link_name_count == max_invalid_name_prints ) {
        printf("WARNING: Number of invalid link names exceeds limit of %d, no more messages will be printed\n",
            max_invalid_name_prints);
        bad_link_name_count++;
    }
}
} // anonymous namespace


namespace SST {

ConfigGraph::ConfigGraph() :
    nextComponentId(0)
{
    links_.clear();
    comps_.clear();
    // Init the statistic output settings
    stats_config_             = new StatsConfig();
    stats_config_->load_level = STATISTICSDEFAULTLOADLEVEL;
    stats_config_->outputs.emplace_back(STATISTICSDEFAULTOUTPUTNAME);
    // Output is only used for warnings or fatal that should go to stderr
    Output& o = Output::getDefaultObject();
    output.init(o.getPrefix(), o.getVerboseLevel(), o.getVerboseMask(), Output::STDERR);
}

ConfigGraph::~ConfigGraph()
{
    for ( auto comp : comps_ ) {
        delete comp;
    }

    if ( stats_config_ ) delete stats_config_;
}

std::vector<ConfigStatOutput>&
ConfigGraph::getStatOutputs()
{
    return stats_config_->outputs;
}

const ConfigStatOutput&
ConfigGraph::getStatOutput(size_t index) const
{
    return stats_config_->outputs[index];
}

long
ConfigGraph::getStatLoadLevel() const
{
    return stats_config_->load_level;
}

const std::map<std::string, ConfigStatGroup>&
ConfigGraph::getStatGroups() const
{
    return stats_config_->groups;
}

ConfigStatGroup*
ConfigGraph::getStatGroup(const std::string& name)
{
    auto found = stats_config_->groups.find(name);
    if ( found == stats_config_->groups.end() ) {
        bool ok;
        std::tie(found, ok) = stats_config_->groups.emplace(name, name);
    }
    return &(found->second);
}


size_t
ConfigGraph::getNumComponentsInMPIRank(uint32_t rank)
{
    size_t count = 0;
    for ( auto* comp : comps_ ) {
        if ( comp->rank.rank == rank ) ++count;
    }
    return count;
}


void
ConfigGraph::setComponentRanks(RankInfo rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps_.begin(); iter != comps_.end(); ++iter ) {
        (*iter)->setRank(rank);
    }
}

bool
ConfigGraph::containsComponentInRank(RankInfo rank)
{
    for ( ConfigComponentMap_t::iterator iter = comps_.begin(); iter != comps_.end(); ++iter ) {
        if ( (*iter)->rank == rank ) return true;
    }
    return false;
}

bool
ConfigGraph::checkRanks(RankInfo ranks)
{
    for ( auto& comp : comps_ ) {
        if ( !comp->rank.isAssigned() || !ranks.inRange(comp->rank) ) {
            fprintf(stderr, "Bad rank: %u %u\n", comp->rank.rank, comp->rank.thread);
            return false;
        }
    }

    // Set the cross_rank and cross_thread flags
    for ( auto& link : links_ ) {
        RankInfo r0(-1, -1);
        RankInfo r1(-1, -1);
        r0 = comps_[COMPONENT_ID_MASK(link->component[0])]->rank;

        if ( link->nonlocal ) {
            r1.rank   = link->component[1];
            r1.thread = link->latency[1];
        }
        else {
            r1 = comps_[COMPONENT_ID_MASK(link->component[1])]->rank;
        }

        if ( r0.rank != r1.rank ) {
            link->cross_rank = true;
        }
        else if ( r0.thread != r1.thread ) {
            link->cross_thread = true;
        }
    }

    return true;
}

void
ConfigGraph::postCreationCleanup()
{
    for ( ConfigLink* link : getLinkMap() ) {
        link->updateLatencies();
    }

    // Need to assign the link delivery order.  This is done
    // alphabetically by link name. To save memory, we'll sort links_
    // by name, then sort it back by link_id
    // std::sort(links_.begin(), links_.end(),
    //     [](const ConfigLink* lhs, const ConfigLink* rhs) -> bool { return lhs->name < rhs->name; });

    // LinkId_t count = 1;
    // for ( auto* link : links_ ) {
    //     link->order = count;
    //     count++;
    // }

    // links_.sort();

    /* Force component / statistic registration for Group stats */
    for ( auto& cfg : getStatGroups() ) {
        for ( ComponentId_t compID : cfg.second.components ) {
            ConfigComponent* ccomp = findComponent(compID);
            if ( ccomp ) { /* Should always be true */
                for ( auto& kv : cfg.second.statMap ) {
                    ccomp->enableStatistic(kv.first, kv.second);
                }
            }
        }
    }
}

// Checks for errors that can't be easily detected during the build
// process
bool
ConfigGraph::checkForStructuralErrors()
{
    // Check to make sure there are no dangling links.  A dangling
    // link is found by looking though the links in the graph and
    // making sure there are components on both sides of the link.

    bool found_error = false;
    for ( ConfigLinkMap_t::iterator iter = links_.begin(); iter != links_.end(); ++iter ) {
        ConfigLink* clink = *iter;

        // First check to see if the link is completely unused
        if ( clink->order == 0 ) {
            output.output("WARNING:  Found unused link: %s\n", clink->name.c_str());
            found_error = true;
        }

        // If component[0] is not initialized, this is an unused link
        if ( clink->component[0] == ULONG_MAX ) {
            output.output("WARNING:  Found unused link: %s\n", clink->name.c_str());
            found_error = true;
        }
        // If component[1] is not initialized, this is a dangling link
        else if ( clink->component[1] == ULONG_MAX ) {
            output.output("WARNING:  Found dangling link: %s.  It is connected on one side to component %s.\n",
                clink->name.c_str(), comps_[clink->component[0]]->name.c_str());
            found_error = true;
        }
    }

    // Check to see if all the port names are valid and they are only
    // used once

    // Loop over all the Components
    for ( ConfigComponentMap_t::iterator iter = comps_.begin(); iter != comps_.end(); ++iter ) {
        ConfigComponent* ccomp = *iter;
        ccomp->checkPorts();
    }

    return found_error;
}

ComponentId_t
ConfigGraph::addComponent(const std::string& name, const std::string& type)
{
    checkForValidComponentName(name);
    ComponentId_t cid = nextComponentId++;
    comps_.insert(new ConfigComponent(cid, this, name, type, 1.0f, RankInfo()));

    auto ret = comps_by_name_.insert(std::make_pair(name, cid));
    // Check to see if the name has already been used
    if ( !ret.second ) {
        output.fatal(CALL_INFO, 1, "ERROR: trying to add Component with name that already exists: %s\n", name.c_str());
    }
    return cid;
}

void
ConfigGraph::addSharedParam(const std::string& shared_set, const std::string& key, const std::string& value)
{
    Params::insert_shared(shared_set, key, value);
}

void
ConfigGraph::setStatisticOutput(const std::string& name)
{
    stats_config_->outputs[0].type = name;
}

void
ConfigGraph::setStatisticOutputParams(const Params& p)
{
    stats_config_->outputs[0].params = p;
}

void
ConfigGraph::addStatisticOutputParameter(const std::string& param, const std::string& value)
{
    stats_config_->outputs[0].params.insert(param, value);
}

void
ConfigGraph::setStatisticLoadLevel(uint8_t loadLevel)
{
    stats_config_->load_level = loadLevel;
}

void
ConfigGraph::addLink(ComponentId_t comp_id, LinkId_t link_id, const char* port, const char* latency_str)
{
    // checkForValidLinkName(link_name);

    // The Link was created earlier, just get it out of the links_
    // data structure.
    ConfigLink* link = links_[link_id];

    // Check to make sure the link has not been referenced too many
    // times.
    if ( link->order >= 2 ) {
        output.fatal(
            CALL_INFO, 1, "ERROR: Parsing SDL file: Link %s referenced more than two times\n", link->name.c_str());
    }
    else if ( link->order == 1 && link->nonlocal ) {
        output.fatal(CALL_INFO, 1,
            "ERROR: Parsing SDL file: Attempting to connect second component to link %s which is set as non-local\n",
            link->name.c_str());
    }

    // Check to make sure that a latency was specified, either in the
    // call or at ConfigLink construct time
    if ( nullptr == latency_str && link->latency[0] == 0 ) {
        output.fatal(CALL_INFO, 1, "ERROR: Parsing SDL file: Connecting link with no latency assigned: %s\n",
            link->name.c_str());
    }

    // Update link information
    int index              = link->order++;
    link->component[index] = comp_id;
    link->port[index]      = port;

    // A nullptr for latency_str means use the latency specified at
    // link creation
    if ( latency_str ) link->latency[index] = ConfigLink::getIndexForLatency(latency_str);

    // Need to add this link to the ConfigComponent's link list.
    // Check to make sure the link doesn't already exist in the
    // component.  Only possible way it could be there is if the link
    // is attached to the component at both ends.  So, if this is the
    // first reference to the link, or if link->component[0] is not
    // equal to the current component sent into this call, then it is
    // not already in the list.
    if ( link->order == 1 || link->component[0] != comp_id ) {
        auto compLinks = &findComponent(comp_id)->links;
        compLinks->push_back(link->id);
    }
}

void
ConfigGraph::addNonLocalLink(LinkId_t link_id, int rank, int thread)
{
    ConfigLink* link = links_[link_id];
    if ( link->nonlocal ) {
        output.fatal(CALL_INFO, 1,
            "ERROR: Parsing SDL file: Trying to set link %s as as non-local, which is already set to non-local\n",
            link->name.c_str());
    }
    else if ( link->order == 2 ) {
        output.fatal(CALL_INFO, 1,
            "ERROR: Parsing SDL file: Link %s being set as non-local, but is already connected to two components\n",
            link->name.c_str());
    }
    link->nonlocal     = true;
    link->component[1] = rank;
    link->latency[1]   = thread;
}


LinkId_t
ConfigGraph::createLink(const char* name, const char* latency)
{
    checkForValidLinkName(name);
    LinkId_t    id   = (LinkId_t)links_.size();
    ConfigLink* link = new ConfigLink(id, name);
    links_.insert(link);
    if ( latency ) {
        uint32_t index   = ConfigLink::getIndexForLatency(latency);
        link->latency[0] = index;
        link->latency[1] = index;
    }
    return id;
}

void
ConfigGraph::setLinkNoCut(LinkId_t link_id)
{
    links_[link_id]->no_cut = true;
}

bool
ConfigGraph::containsComponent(ComponentId_t id) const
{
    return comps_.contains(id);
}

ConfigComponent*
ConfigGraph::findComponent(ComponentId_t id)
{
    return const_cast<ConfigComponent*>(const_cast<const ConfigGraph*>(this)->findComponent(id));
}

const ConfigComponent*
ConfigGraph::findComponent(ComponentId_t id) const
{
    /* Check to make sure we're part of the same component */
    if ( COMPONENT_ID_MASK(id) == id ) {
        return comps_[id];
    }

    return comps_[COMPONENT_ID_MASK(id)]->findSubComponent(id);
}

ConfigComponent*
ConfigGraph::findComponentByName(const std::string& name)
{
    std::string origname(name);
    auto        index    = origname.find(':');
    std::string compname = origname.substr(0, index);
    auto        itr      = comps_by_name_.find(compname);

    // Check to see if component was found
    if ( itr == comps_by_name_.end() ) return nullptr;

    ConfigComponent* cc = comps_[itr->second];

    // If this was just a component name
    if ( index == std::string::npos ) return cc;

    // See if this is a valid subcomponent name
    cc = cc->findSubComponentByName(origname.substr(index + 1, std::string::npos));
    if ( cc ) return cc;
    return nullptr;
}

ConfigStatistic*
ConfigGraph::findStatistic(StatisticId_t id) const
{
    ComponentId_t cfg_id = CONFIG_COMPONENT_ID_MASK(id);
    return findComponent(cfg_id)->findStatistic(id);
}


ConfigGraph::GraphFilter::GraphFilter(ConfigGraph* original_graph, ConfigGraph* new_graph,
    const std::set<uint32_t>& original_rank_set, const std::set<uint32_t>& new_rank_set) :
    ograph_(original_graph),
    ngraph_(new_graph),
    oset_(original_rank_set),
    nset_(new_rank_set)
{}

ConfigLink*
ConfigGraph::GraphFilter::operator()(ConfigLink* link)
{
    // Need to see if the link is connected to components in the
    // old and/or new graph
    RankInfo ranks[2];
    ranks[0] = ograph_->findComponent(link->component[0])->rank;
    if ( link->nonlocal ) {
        ranks[1].rank = -1;
    }
    else {
        ranks[1] = ograph_->findComponent(link->component[1])->rank;
    }

    // Check to see which components are in which sets
    bool c0_in_orig = oset_.count(ranks[0].rank);
    bool c1_in_orig = oset_.count(ranks[1].rank);
    bool c0_in_new  = nset_.count(ranks[0].rank);
    bool c1_in_new  = nset_.count(ranks[1].rank);

    // First bit of flag checks to see if either end is in the set
    // that will stay in the original graph
    uint8_t flag = c0_in_orig | c1_in_orig;

    // Second bit will check for the set that will be in the new graph
    flag |= ((c0_in_new | c1_in_new) << 1);

    switch ( flag ) {
    case 0:
        // Not connected in either of the partitions.  This shouldn't
        // happen, but if it did, it's just an extraneous link.  Just
        // delete it and remove from the original graph.
        delete link;
        return nullptr;
    case 1:
        // Connected in original graph, not in new. Just return it

        // See if link needs to be set as nonlocal
        if ( !link->nonlocal && (c0_in_orig ^ c1_in_orig) ) {
            // Only one side is in the set. Figure out which one and set the link as nonlocal
            int index = c0_in_orig ? 0 : 1;
            link->setAsNonLocal(index, ranks[(index + 1) % 2]);
        }
        return link;
    case 2:
        // Connected in new graph, but not original.  Move to new
        // graph
        ngraph_->links_.insert(link);

        // See if link needs to be set as nonlocal
        if ( !link->nonlocal && (c0_in_new ^ c1_in_new) ) {
            // Only one side is in the set. Figure out which one and set the link as nonlocal
            int index = c0_in_new ? 0 : 1;
            link->setAsNonLocal(index, ranks[(index + 1) % 2]);
        }
        return nullptr;
    case 3:
        // Connected in both graphs.  Make a copy for the new graph and mark both links as cross partition.  NOTE: we
        // won't get to this state unless the graph originally used a ghost component for the cross partition
        // link. Links marked as cross platform can't get here because ranks[1] is set to -1.
        ConfigLink* link_new = new ConfigLink(*link);
        ngraph_->links_.insert(link_new);

        // Now change the two links to cross partition
        if ( c0_in_new ) {
            // component[0] in new set
            link->setAsNonLocal(1, ranks[0]);
            link_new->setAsNonLocal(0, ranks[1]);
        }
        else {
            // component[1] in new set
            link->setAsNonLocal(0, ranks[1]);
            link_new->setAsNonLocal(1, ranks[0]);
        }

        return link;
    }
    // Silence warning even though every possible path has a return
    return nullptr;
}

// Must run the link filter first!
ConfigComponent*
ConfigGraph::GraphFilter::operator()(ConfigComponent* comp)
{
    // Figure out which graph it belongs in and put it there.  All the
    // cross partition info is now held in ConfigLink

    if ( oset_.count(comp->rank.rank) ) {
        // Stays in the current graph
        return comp;
    }
    else if ( nset_.count(comp->rank.rank) ) {
        // Move to new graph
        comp->graph = ngraph_;
        ngraph_->comps_.insert(comp);
        return nullptr;
    }
    else {
        // Not in either group.  Need to delete comp and return
        // nullptr.  This should only happen if the user used ghost
        // components to specify remote rank
        delete comp;
        return nullptr;
    }
}


ConfigGraph*
ConfigGraph::splitGraph(const std::set<uint32_t>& orig_rank_set, const std::set<uint32_t>& new_rank_set)
{
    ConfigGraph* graph = nullptr;

    if ( !new_rank_set.empty() ) {
        graph = new ConfigGraph();

        // Need to copy over any restart data
        graph->cpt_ranks           = cpt_ranks;
        graph->cpt_currentSimCycle = cpt_currentSimCycle;
        graph->cpt_currentPriority = cpt_currentPriority;
        graph->cpt_minPart         = cpt_minPart;
        graph->cpt_minPartTC       = cpt_minPartTC;
        graph->cpt_max_event_id    = cpt_max_event_id;

        graph->cpt_libnames       = cpt_libnames;
        graph->cpt_shared_objects = cpt_shared_objects;
        graph->cpt_stats_config   = cpt_stats_config;
    }

    // Split up the links
    GraphFilter filter(this, graph, orig_rank_set, new_rank_set);
    links_.filter(filter);

    // Split up the components
    comps_.filter(filter);

    // Copy the statistic configuration to the sub-graph
    if ( graph ) graph->stats_config_->outputs = this->stats_config_->outputs;

    // Need to copy statgroups contained in new graph and remove statgroups that are no longer needed in original graph.
    // However, for now, we need to leave all stat group information on rank 0 so that all statgroups get checkpointed
    // (only rank 0 adds statgroups to the checkpoint)

    bool orig_includes_zero = orig_rank_set.count(0);
    bool new_includes_zero  = new_rank_set.count(0);
    for ( auto it = this->stats_config_->groups.begin(); it != this->stats_config_->groups.end();
        /* increment in loop body */ ) {
        bool copy   = false;
        bool remove = true;
        for ( auto& id : it->second.components ) {
            if ( (graph && graph->containsComponent(id)) || new_includes_zero ) {
                copy = true;
                // We can stop if we've verified it belongs in both
                // already.  In this case, if remove is false, then we
                // can break
                if ( !remove ) break;
            }
            if ( containsComponent(id) || orig_includes_zero ) {
                remove = false;
                // We can stop if we've verified it belongs in both
                // already.  In this case, if copy is true, then we
                // can break
                if ( (nullptr == graph) || copy ) break;
            }
        }

        // See if we need to copy into new graph
        if ( copy ) { // If graph is nullptr, then copy can't be true
            graph->stats_config_->groups.insert(std::make_pair(it->first, it->second));
        }

        // See if we need to remove from the original graph.
        if ( remove ) {
            it = this->stats_config_->groups.erase(it);
        }
        else {
            ++it;
        }
    }

    if ( graph ) graph->setStatisticLoadLevel(this->getStatLoadLevel());


    return graph;
}

void
ConfigGraph::reduceGraphToSingleRank(uint32_t rank)
{
    std::set<uint32_t> ranks = { rank };
    splitGraph(ranks, std::set<uint32_t>());
}

SimTime_t
ConfigGraph::getMinimumPartitionLatency()
{
    if ( getNumComponents() == 0 ) {
        // This is a restart run with no repartitioning, so just return the minPart from the checkpoint
        return cpt_minPart;
    }

    SimTime_t graph_min_part = std::numeric_limits<SimTime_t>::max();

    for ( auto& link : links_ ) {
        if ( link->cross_rank ) {
            SimTime_t min_lat = link->getMinLatency();
            if ( min_lat < graph_min_part ) {
                graph_min_part = min_lat;
            }
        }
    }
    return graph_min_part;
}


PartitionGraph*
ConfigGraph::getPartitionGraph()
{
    PartitionGraph* graph = new PartitionGraph();

    PartitionComponentMap_t& pcomps = graph->getComponentMap();
    PartitionLinkMap_t&      plinks = graph->getLinkMap();

    // SparseVectorMap is slow for random inserts, so make sure we
    // insert both components and links in order of ID, which is the
    // key for the SparseVectorMap
    for ( ConfigComponentMap_t::iterator it = comps_.begin(); it != comps_.end(); ++it ) {
        const ConfigComponent* comp = *it;

        pcomps.insert(new PartitionComponent(comp));
    }

    for ( ConfigLinkMap_t::iterator it = links_.begin(); it != links_.end(); ++it ) {
        const ConfigLink* link = *it;

        const ConfigComponent* comp0 = comps_[COMPONENT_ID_MASK(link->component[0])];
        const ConfigComponent* comp1 = comps_[COMPONENT_ID_MASK(link->component[1])];

        plinks.insert(PartitionLink(*link));

        pcomps[comp0->id]->links.push_back(link->id);
        pcomps[comp1->id]->links.push_back(link->id);
    }
    return graph;
}

PartitionGraph*
ConfigGraph::getCollapsedPartitionGraph()
{
    PartitionGraph* graph = new PartitionGraph();

    std::set<LinkId_t> deleted_links;

    PartitionComponentMap_t& pcomps = graph->getComponentMap();
    PartitionLinkMap_t&      plinks = graph->getLinkMap();

    // Mark all Components as not visited
    for ( ConfigComponentMap_t::iterator it = comps_.begin(); it != comps_.end(); ++it )
        (*it)->visited = false;

    // SparseVectorMap is slow for random inserts, so make sure we
    // insert both components and links in order of ID, which is the
    // key for the SparseVectorMap in both cases

    // Use an ordered set so that when we insert the ids for the group
    // into a SparseVectorMap, we are inserting in order.
    std::set<ComponentId_t> group;
    for ( ConfigComponentMap_t::iterator it = comps_.begin(); it != comps_.end(); ++it ) {
        auto comp = *it;
        // If this component ended up in a connected group we already
        // looked at, skip it
        if ( comp->visited ) continue;

        // Get the no-cut group for this component
        group.clear();
        getConnectedNoCutComps(comp->id, group);

        // Create a new PartitionComponent for this group
        ComponentId_t       id    = pcomps.size();
        PartitionComponent* pcomp = pcomps.insert(new PartitionComponent(id));

        // Iterate over the group and add the weights and add any
        // links that connect outside the group
        for ( std::set<ComponentId_t>::const_iterator i = group.begin(); i != group.end(); ++i ) {
            const ConfigComponent* comp = comps_[*i];
            // Compute the new weight
            pcomp->weight += comp->weight;
            // Inserting in order because the iterator is from an
            // ordered set
            pcomp->group.insert(*i);

            // Walk through all the links and insert the ones that connect
            // outside the group
            for ( LinkId_t id : comp->allLinks() ) {
                const ConfigLink* link = links_[id];

                if ( group.find(COMPONENT_ID_MASK(link->component[0])) == group.end() ||
                     group.find(COMPONENT_ID_MASK(link->component[1])) == group.end() ) {
                    pcomp->links.push_back(link->id);
                }
                else {
                    deleted_links.insert(link->id);
                }
            }
        }
    }

    // Now add all but the deleted links to the partition graph.  We
    // do it here so that we insert in order because we are using a
    // SparseVectorMap.  It may look like we are inserting the actual
    // ConfigLink into the map, but this actually adds a PartitionLink
    // to the set by passing the ConfigLink into the constructor.
    // This will insert in order since the iterator is from a
    // SparseVectorMap.
    for ( ConfigLinkMap_t::iterator i = links_.begin(); i != links_.end(); ++i ) {
        if ( deleted_links.find((*i)->id) == deleted_links.end() ) plinks.insert(*(*i));
    }

    // Just need to fix up the component fields for the links.  Do
    // this by walking through the components and checking each of it's
    // links to see if it points to something in the group.  If so,
    // change ID to point to super group.
    for ( PartitionComponentMap_t::iterator i = pcomps.begin(); i != pcomps.end(); ++i ) {
        PartitionComponent* pcomp = *i;
        for ( LinkIdMap_t::iterator j = pcomp->links.begin(); j != pcomp->links.end(); ++j ) {
            PartitionLink& plink = plinks[*j];
            if ( pcomp->group.contains(plink.component[0]) ) plink.component[0] = pcomp->id;
            if ( pcomp->group.contains(plink.component[1]) ) plink.component[1] = pcomp->id;
        }
    }

    return graph;
}

void
ConfigGraph::annotateRanks(PartitionGraph* graph)
{
    PartitionComponentMap_t& pcomps = graph->getComponentMap();

    for ( PartitionComponentMap_t::iterator it = pcomps.begin(); it != pcomps.end(); ++it ) {
        const PartitionComponent* comp = *it;

        for ( ComponentIdMap_t::const_iterator c_iter = comp->group.begin(); c_iter != comp->group.end(); ++c_iter ) {
            comps_[*c_iter]->setRank(comp->rank);
        }
    }
}

void
ConfigGraph::getConnectedNoCutComps(ComponentId_t start, std::set<ComponentId_t>& group)
{
    // We'll do this as a simple recursive depth first search
    group.insert(COMPONENT_ID_MASK(start));

    // First, get the component
    ConfigComponent* comp = comps_[start];
    comp->visited         = true;

    for ( LinkId_t id : comp->allLinks() ) {
        ConfigLink* link = links_[id];

        // If this is a no_cut link, need to follow it to next
        // component if next component is not already in group
        if ( link->no_cut ) {
            ComponentId_t id = COMPONENT_ID_MASK(
                (COMPONENT_ID_MASK(link->component[0]) == COMPONENT_ID_MASK(start) ? link->component[1]
                                                                                   : link->component[0]));
            // Check to see if this id is already in the group.  We
            // can do it one of two ways: check the visited variable,
            // or see if it is in the group set already.  We look in
            // the group set because they are both lookups into
            // associative structures, but the group will be much
            // smaller.
            if ( group.find(id) == group.end() ) {
                getConnectedNoCutComps(id, group);
            }
        }
    }
}

void
ConfigGraph::setComponentConfigGraphPointers()
{
    for ( auto* x : comps_ ) {
        x->setConfigGraphPointer(this);
    }
}

void
ConfigGraph::restoreRestartData()
{
    SST::Core::Serialization::serializer ser;
    ser.enable_pointer_tracking();

    // Load libraries from the checkpoint
    Factory::getFactory()->loadUnloadedLibraries(*(cpt_libnames.get()));

    // Initialize SharedObjectManager
    auto* vec_som = cpt_shared_objects.get();
    ser.start_unpacking(vec_som->data(), vec_som->size());
    Simulation_impl::serializeSharedObjectManager(ser);

    // Get the stats config
    auto* vec_sc = cpt_stats_config.get();
    ser.start_unpacking(vec_sc->data(), vec_sc->size());
    SST_SER(Simulation_impl::stats_config_);

    cpt_libnames.reset();
    cpt_shared_objects.reset();
    cpt_stats_config.reset();
}

void
PartitionComponent::print(std::ostream& os, const PartitionGraph* graph) const
{
    os << "Component " << id << "  ( ";
    for ( ComponentIdMap_t::const_iterator git = group.begin(); git != group.end(); ++git ) {
        os << *git << " ";
    }
    os << ")" << std::endl;
    os << "  weight = " << weight << std::endl;
    os << "  rank = " << rank.rank << std::endl;
    os << "  thread = " << rank.thread << std::endl;
    os << "  Links:" << std::endl;
    for ( LinkIdMap_t::const_iterator it = links.begin(); it != links.end(); ++it ) {
        graph->getLink(*it).print(os);
    }
}

} // namespace SST
