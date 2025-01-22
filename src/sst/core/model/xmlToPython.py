#!/usr/bin/env python

# Copyright 2009-2025 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2025, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

import xml.etree.ElementTree as ET
import sys, os, re
from typing import Dict

import sst

def printTree(indent: int, node: ET.Element) -> None:
    print("%sBegin %s: %r"%('  '*indent, node.tag, node.attrib))
    if node.text and len(node.text.strip()):
        print("%sText:  %s"%('  '*indent, node.text.strip()))
    for child in node:
        printTree(indent+1, child)
    print("%sEnd %s"%('  '*indent, node.tag))



# Various global lookups
sstVars: Dict[str, str] = dict()
sstParams = dict()
sstLinks = dict()


# Some regular expressions
sdlRE = re.compile("<sdl([^/]*?)/>")
commentRE = re.compile("<!--.*?-->", re.DOTALL)
eqRE = re.compile(r"(<[^>]+?\w+)=([^\"\'][^\\s/>]*)")  # This one is suspect
namespaceRE = re.compile(r"<\s*((\w+):\w+)")

envVarRE = re.compile("\\${(.*?)}", re.DOTALL)
sstVarRE = re.compile("\\$([^{][a-zA-Z0-9_]+)", re.DOTALL)


def processString(string: str) -> str:
    """Process a string, replacing variables and env. vars with their values"""
    def replaceSSTVar(matchobj: re.Match[str]) -> str:
        varname = matchobj.group(1)
        return sstVars[varname]

    def replaceEnvVar(matchobj: re.Match[str]) -> str:
        varname = matchobj.group(1)
        var = os.getenv(varname)
        assert var is not None
        return var

    string = envVarRE.sub(replaceEnvVar, string)
    string = sstVarRE.sub(replaceSSTVar, string)
    return string


def getLink(name: str) -> sst.Link:
    if name not in sstLinks:
        sstLinks[name] = sst.Link(name)
    return sstLinks[name]



def getParamName(node: ET.Element) -> str:
    name = node.tag.strip()
    if name[0] == "{":
        ns, tag = name[1:].split("}")
        return ns + ":" + tag
    return name


def processParamSets(groups: ET.Element) -> None:
    for group in groups:
        params = dict()
        for p in group:
            params[getParamName(p)] = processString(p.text.strip())  # type: ignore [union-attr]
        sstParams[group.tag] = params


def processVars(varNode: ET.Element) -> None:
    for var in varNode:
        sstVars[var.tag] = processString(var.text.strip())  # type: ignore [union-attr]

def processConfig(cfg: ET.Element) -> None:
    for line in cfg.text.strip().splitlines():  # type: ignore [union-attr]
        var, val = line.split('=')
        sst.setProgramOption(var, processString(val)) # strip quotes



def buildComp(compNode: ET.Element) -> None:
    name = processString(compNode.attrib['name'])
    type = processString(compNode.attrib['type'])
    comp = sst.Component(name, type)

    # Process Parameters
    paramsNode = compNode.find("params")
    params = dict()
    if paramsNode is not None:
        if "include" in paramsNode.attrib:
            for paramInc in paramsNode.attrib['include'].split(','):
                params.update(sstParams[processString(paramInc)])
        for p in paramsNode:
            params[getParamName(p)] = processString(p.text.strip())  # type: ignore [union-attr]

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



def buildGraph(graph: ET.Element) -> None:
    for comp in graph.findall("component"):
        buildComp(comp)



def build(root: ET.Element) -> None:
    paramSets = root.find("param_include")
    variables = root.find("variables")
    cfg = root.find("config")
    timebase = root.find("timebase")
    graph = root.find("sst")


    if variables is not None:
        processVars(variables)
    if paramSets is not None:
        processParamSets(paramSets)
    if timebase is not None:
        sst.setProgramOption('timebase', timebase.text.strip())  # type: ignore [union-attr]
    if cfg is not None:
        processConfig(cfg)
    if graph is not None:
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
        print("Unknown format")
        sst.exit(1)
