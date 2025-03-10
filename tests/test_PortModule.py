#!/usr/bin/env python
#
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

import sst
import argparse
import sys

def main() -> None:
    parser = argparse.ArgumentParser(description="Run PortModule test")
    parser.add_argument('--send', action='store_true', help="Install PortModule on send")
    parser.add_argument('--recv', action='store_true', help="Install PortModule on receive")
    parser.add_argument('--subcomp', action='store_true', help="Use SubComponent to configure ports")
    parser.add_argument('--pass', action='store_true', dest="my_pass", help="Pass event through unchanged")
    parser.add_argument('--drop', action='store_true', help="Drop event")
    parser.add_argument('--modify', action='store_true', help="Modify event")
    parser.add_argument('--replace', action='store_true', help="Replace event with an ack event")
    parser.add_argument('--randomdrop', action='store_true', help="Use sst.portmodules.random_drop port module")
    args = parser.parse_args()

    if not args.send ^ args.recv:
        print("ERROR: Must enable one, and only one, of --send or --recv")
        sys.exit()

    count = 0
    if args.my_pass: count += 1
    if args.drop: count += 1
    if args.modify: count += 1
    if args.replace: count += 1
    if args.randomdrop: count += 1
    if count != 1:
        print("ERROR: Must enable one, and only one, of --pass, --drop, --modify, --replace or --randomdrop")
        sys.exit()

    num_comps = 12
    
    params = { "use_subcomponent" : args.subcomp,
               "repeat_last" : args.randomdrop }
    
    comp = sst.Component("comp0", "coreTestElement.coreTestPortModuleComponent")
    comp.addParams(params)
    for x in range(num_comps - 2):
        comp2 = sst.Component("comp{0}".format(x+1),"coreTestElement.coreTestPortModuleComponent")
        comp2.addParams(params)
        link = sst.Link("link_{0}".format(x))
        link.connect((comp,"right","1ns"),(comp2,"left","1ns"))
        comp = comp2

    comp2 = sst.Component("comp{0}".format(num_comps-1),"coreTestElement.coreTestPortModuleComponent")
    comp2.addParams(params)
    link = sst.Link("link_{0}".format(num_comps-1))
    link.connect((comp,"right","1ns"),(comp2,"left","1ns"))
    
    # Add the PortModule
    if args.randomdrop:
        if args.recv:
            comp2.addPortModule("left", "sst.portmodules.random_drop", {
                "drop_prob": 0.5,
                "drop_on_send": False,
                "verbose" : True
            })
        else:
            comp.addPortModule("right", "sst.portmodules.random_drop", {
                "drop_prob": 0.5,
                "drop_on_send": True,
                "verbose" : True
            })
    else:
        if args.recv:
            comp2.addPortModule("left", "coreTestElement.portmodules.test", {
                "modify": args.modify,
                "drop": args.drop,
                "replace": args.replace,
                "install_on_send": False
            })
        else:
            comp.addPortModule("right", "coreTestElement.portmodules.test", {
                "modify": args.modify,
                "drop": args.drop,
                "replace": args.replace,
                "install_on_send": True
            })

if __name__ == "__main__":
    main()
