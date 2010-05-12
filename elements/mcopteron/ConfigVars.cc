
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ConfigVars.h>

#ifdef TESTING
#define EXTERN 
#endif
#include <OpteronDefs.h>

ConfigVars::ConfigVars(const char *configFilename)
{
   domains  =  0;
   domainInUse  = 0;
   addDomain("_root_");
   if (configFilename)
      readConfigFile(configFilename);
}

ConfigVars::~ConfigVars()
{
   Domain *d, *dt;
   Variable *v, *vt;
   d = domains;
   while (d) {
      v = d->vars;
      if (Debug>1)
         fprintf(debugLogFP, "deleting config domain (%s)\n", d->name);
      while (v) {
         if (Debug>1)
            fprintf(debugLogFP, "   deleting variable (%s)=(%s)\n", v->name, v->value);
         if (v->name) free((void*)v->name);
         if (v->value) free((void*)v->value);
         vt = v;
         v = v->next;
         delete vt;
      }
      if (d->name) free((void*)d->name);
      dt = d;
      d = d->next;
      delete dt;
   }
}

int ConfigVars::readConfigFile(const char *filename)
{
   FILE *inf;
   char line[512];
   int n;
   char name[128], value[128];
   if (!filename)
      return 1;
   inf = fopen(filename, "r");
   if (!inf)
      return 2;
   if (Debug>1)
      fprintf(debugLogFP, "reading config file (%s)\n", filename);
   while (fgets(line, sizeof(line), inf) != 0) {
      if (line[0] == '#' || line[0] == '/' || line[0] == '\n' || line[0] == '\0')
         // assume a comment or blank, so skip
         continue;
      n = sscanf(line, "[%[^] \t]", (char*) &name);
      if (n == 1) {
         // a new domain specified
         addDomain(name);
         continue;
      }
      n = sscanf(line, "%[^= \t] = %s", (char*) &name, (char*) &value);
      if (n == 2) {
         // a new variable specified
         addVariable(name, value);
         continue;
      }
      n = sscanf(line, "%[^= \t]=%s", (char*) &name, (char*) &value);
      if (n == 2) {
         // a new variable specified
         addVariable(name, value);
         continue;
      }
      if (Debug>1) fprintf(debugLogFP, "  unknown line (%s)\n", line);
   }
   fclose(inf);
   useDomain("_root_");
   return 0;
}

int ConfigVars::addDomain(const char *domain)
{
   Domain *d;
   if (!domain)
      return 1;
   if (Debug>1) fprintf(debugLogFP, "adding config domain (%s)\n", domain);
   if (!useDomain(domain)) {
      if (Debug>1) fprintf(debugLogFP, "  domain exists\n");
      // already exists, so don't add
      return 1;
   }
   if (Debug>1) fprintf(debugLogFP, "  new domain (%s)\n", domain);
   d = new Domain();
   d->name = strdup(domain);
   d->next = domains;
   domains = d;
   domainInUse = d;
   return 0;
}

int ConfigVars::useDomain(const char *domain)
{
   Domain *d = domains;
   if (!domain)
      domain = "_root_";
   if (Debug>1) fprintf(debugLogFP, "using config domain (%s)...", domain);
   while (d) {
      if (!strcmp(domain, d->name))
         break;
      d = d->next;
   }
   if (Debug>1) fprintf(debugLogFP, "(%p)\n", d);
   if (!d)
      return 1;
   domainInUse = d;
   return 0;
}

int ConfigVars::addVariable(const char *name, const char *value)
{
   Variable *v;
   if (Debug>1) fprintf(debugLogFP, "adding variable (%s) (%s)\n", name, value);
   if (!domainInUse)
      useDomain("_root_");
   if (!domainInUse)
      return 1;
   v = findVariableRec(name);
   if (v) {
      if (Debug>1) fprintf(debugLogFP, "   updating value from (%s)\n", v->value);
      const char *t = v->value;
      v->value = strdup(value);
      if (t) free((void*)t);
      return 0;
   }
   v = new Variable();
   v->name = strdup(name);
   v->value = strdup(value);
   v->next = domainInUse->vars;
   domainInUse->vars = v;
   return 0;
}

const char* ConfigVars::findVariable(const char *name)
{
   Variable *v;
   v = findVariableRec(name);
   if (!v)
      return 0;
   return v->value;
}

ConfigVars::Variable* ConfigVars::findVariableRec(const char *name)
{
   Variable *v;
   if (Debug>1) fprintf(debugLogFP, "finding variable (%s)...", name);
   if (!domainInUse)
      useDomain("_root_");
   if (!domainInUse)
      return 0;
   v = domainInUse->vars;
   while (v) {
      if (!strcmp(v->name, name))
         break;
      v = v->next;
   }
   if (Debug>1) fprintf(debugLogFP, "(%p)\n", v);
   return v;
}

int ConfigVars::removeVariable(const char *name)
{
   return 0;
}

#ifdef TESTING

FILE *debugLogFP = stderr;

int main(int argc, char **argv)
{
   ConfigVars *cv;
   Debug = 2;
   cv = new ConfigVars(argv[1]);
   cv->useDomain("memory");
   fprintf(debugLogFP, "L1: (%s)\n", cv->findVariable("l1latency"));
   delete cv;
   return 0;
}

#endif
