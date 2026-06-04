# -*- coding: utf-8 -*-

# Copyright 2009-2026 NTESS. Under the terms
# of Contract DE-NA0003525 with NTESS, the U.S.
# Government retains certain rights in this software.
#
# Copyright (c) 2009-2026, NTESS
# All rights reserved.
#
# This file is part of the SST software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.

""" This module provides the low level calls to the OS shell and other support
    functions
"""

from typing import Any, Type


################################################################################

def strclass(cls: Type[Any]) -> str:
    """ Return the classname of a class"""
    return "%s" % (cls.__module__)

def strqual(cls: Type[Any]) -> str:
    """ Return the qualname of a class"""
    return "%s" % (cls.__qualname__)
