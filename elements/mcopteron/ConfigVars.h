
#ifndef CONFIGVARS_H
#define CONFIGVARS_H

class ConfigVars
{
 public:
   ConfigVars(const char *configFilename=0);
   ~ConfigVars();
   int readConfigFile(const char *filename);
   int addDomain(const char *domain);
   int useDomain(const char *domain=0);
   int addVariable(const char *name, const char *value);
   const char* findVariable(const char *name);
   int removeVariable(const char *name);
 private:
   struct Variable {
      const char *name;
      const char *value;
      struct Variable *next;
   };
   struct Domain {
      const char *name;
      struct Variable *vars;
      struct Domain *next;
   };
   Variable* findVariableRec(const char *name);
   Domain *domains;
   Domain *domainInUse;
};

#endif
