// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_MODEL_SSTMODEL_H
#define SST_CORE_MODEL_SSTMODEL_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/warnmacros.h"

namespace SST {

class Config;
class ConfigGraph;

/** Base class for Model Generation
 */
class SSTModelDescription
{

public:
    SST_ELI_DECLARE_BASE(SSTModelDescription)
    // declare extern to limit compile times
    SST_ELI_DECLARE_CTOR_EXTERN(const std::string&, int, Config*, double)
    SST_ELI_DECLARE_INFO_EXTERN(
        ELI::ProvidesSimpleInfo<0,bool>,
        ELI::ProvidesSimpleInfo<1,std::vector<std::string>>)

    // Helper functions to pull out ELI data for an element
    static bool                            isElementParallelCapable(const std::string& type);
    static const std::vector<std::string>& getElementSupportedExtensions(const std::string& type);

    SSTModelDescription(Config* cfg);
    virtual ~SSTModelDescription() {};

    /** Create the ConfigGraph
     *
     * This function should be overridden by subclasses.
     *
     * This function is responsible for reading any configuration
     * files and generating a ConfigGraph object.
     */
    virtual ConfigGraph* createConfigGraph() = 0;


protected:
    /** Set a configuration string to update configuration values */
    bool setOptionFromModel(const std::string& entryName, const std::string& value);

    /**
     * Sets the model options field of the Config object.  This has a
     * very narrow use case, which is to set the model_options when
     * the model is just a wrapper to another model type and they need
     * to pass some extra options.
     *
     * @param options String that will be used for model_options.  It
     * will overwrite what is alread there
     */
    void setModelOptions(const std::string& options);

    /**
     * Allows ModelDefinition to set global parameters.
     *
     * @param set Name of the global param set to add key/value pair to
     *
     * @param key Key of key/value pair to be added
     *
     * @param key Value of key/value pair to be added
     *
     * @param overwrite Overwrite existing value if set to true (which is default)
     */
    void insertGlobalParameter(
        const std::string& set, const Params::key_type& key, const Params::key_type& value, bool overwrite = true);


private:
    Config* config;
};

} // namespace SST


// Use this macro to register a model description.  Parallel_capable
// indicates whether this model is able to be use when loading in
// parallel.  The final arguments are optional and are a list of file
// extensions handled by the model.  These are only useful for the
// built-in models as external models will have to use the command
// line option to load them and then the extension will be ignored.
#define SST_ELI_REGISTER_MODEL_DESCRIPTION(cls, lib, name, version, desc, parallel_capable)             \
    SST_ELI_REGISTER_DERIVED(SST::SSTModelDescription, ::cls,lib,name,ELI_FORWARD_AS_ONE(version),desc) \
    SST_ELI_DOCUMENT_SIMPLE_INFO(bool,0,parallel_capable)

#define SST_ELI_DOCUMENT_MODEL_SUPPORTED_EXTENSIONS(...) \
    SST_ELI_DOCUMENT_SIMPLE_INFO(std::vector<std::string>,1,__VA_ARGS__)

#endif // SST_CORE_MODEL_SSTMODEL_H
