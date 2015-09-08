#!/usr/bin/env python

import xml.etree.ElementTree as ET
import sys, os, re

import sst

def printTree(indent, node):
    print "%sBegin %s: %r"%('  '*indent, node.tag, node.attrib)
    if node.text and len(node.text.strip()):
        print "%sText:  %s"%('  '*indent, node.text.strip())
    for child in node:
        printTree(indent+1, child)
    print "%sEnd %s"%('  '*indent, node.tag)



# Various global lookups
sstVars = dict()
sstParams = dict()
sstLinks = dict()


# Some regular expressions
sdlRE = re.compile("<sdl([^/]*?)/>")
commentRE = re.compile("<!--.*?-->", re.DOTALL)
eqRE = re.compile("(<[^>]+?\w+)=([^\"\'][^\\s/>]*)")  # This one is suspect
namespaceRE = re.compile("<\s*((\w+):\w+)")

envVarRE = re.compile("\\${(.*?)}", re.DOTALL)
sstVarRE = re.compile("\\$([^{][a-zA-Z0-9_]+)", re.DOTALL)


def processString(str):
    """Process a string, replacing variables and env. vars with their values"""
    def replaceSSTVar(matchobj):
        varname = matchobj.group(1)
        return sstVars[varname]

    def replaceEnvVar(matchobj):
        varname = matchobj.group(1)
        return os.getenv(varname)

    str = envVarRE.sub(replaceEnvVar, str)
    str = sstVarRE.sub(replaceSSTVar, str)
    return str


def getLink(name):
    if name not in sstLinks:
        sstLinks[name] = sst.Link(name)
    return sstLinks[name]



def getParamName(node):
    name = node.tag.strip()
    if name[0] == "{":
        ns, tag = name[1:].split("}")
        return ns + ":" + tag
    return name


def processParamSets(set):
    for group in set:
        params = dict()
        for p in group:
            params[getParamName(p)] = processString(p.text.strip())
        sstParams[group.tag] = params


def processVars(varNode):
    for var in varNode:
        sstVars[var.tag] = processString(var.text.strip())

def processConfig(cfg):
    for line in cfg.text.strip().splitlines():
        var, val = line.split('=')
        sst.setProgramOption(var, processString(val)) # strip quotes



def buildComp(compNode):
    name = processString(compNode.attrib['name'])
    type = processString(compNode.attrib['type'])
    comp = sst.Component(name, type)

    # Process Parameters
    paramsNode = compNode.find("params")
    params = dict()
    if paramsNode != None:
        if "include" in paramsNode.attrib:
            for paramInc in paramsNode.attrib['include'].split(','):
                params.update(sstParams[processString(paramInc)])
        for p in paramsNode:
            params[getParamName(p)] = processString(p.text.strip())

    comp.addParams(params)

    # Process links
    for linkNode in compNode.findall("link"):
        linkName = processString(linkNode.attrib['name'])
        linkPort = processString(linkNode.attrib['port'])
        linkLat = processString(linkNode.attrib['latency'])
        link = getLink(linkName)
        comp.addLink(link, linkPort, linkLat)


    # Rank/Weight
    if "rank" in compNode.attrib:
        comp.setRank(int(compNode.attrib['rank']))
    if "weight" in compNode.attrib:
        comp.setWeight(float(compNode.attrib['rank']))



def buildGraph(graph):
    for comp in graph.findall("component"):
        buildComp(comp)



def build(root):
    paramSets = root.find("param_include")
    vars = root.find("variables")
    cfg = root.find("config")
    timebase = root.find("timebase")
    graph = root.find("sst")


    if None != vars:
        processVars(vars)
    if None != paramSets:
        processParamSets(paramSets)
    if None != timebase:
        sst.setProgramOption('timebase', timebase.text.strip())
    if None != cfg:
        processConfig(cfg)
    if None != graph:
        buildGraph(graph)




xmlFile = sys.argv[1]

with open(xmlFile, 'r') as f:
    data = f.read() + "\n</sdl>"
    # Perform manipulations
    data = sdlRE.sub("<sdl\\1 >", data)
    data = commentRE.sub("", data)
    count = 1
    while count > 0:
        (data, count) = eqRE.subn(r'\1="\2"', data)
    data = namespaceRE.sub("<\\1 xmlns:\\2=\"\\2\"", data)
    data = data.strip()

    # Parse XML
    root = ET.fromstring(data)

    # Process
    if root.tag == 'sdl':
        #printTree(0, root)
        build(root)
    else:
        print "Unknown format"
        sst.exit(1)
