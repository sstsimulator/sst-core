# -*- coding: utf-8 -*-

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

""" This module provides the low level calls to the OS shell and other support
    functions
"""

import inspect
from typing import Any, Type


def check_param_type(varname: str, vardata: Any, datatype: Type[Any]) -> None:
    """ Validate a parameter to ensure it is of the correct type.

        Args:
            varname (str) = The string name of the variable
            vardata (???) = The actual variable of any type
            datatype (???) = The type that we want to confirm

        Raises:
            ValueErr: variable is not of the correct type.
    """
    caller = inspect.stack()[1][3]
    if not isinstance(vardata, datatype):
        err_str = (("TEST-ERROR: {0}() param {1} = {2} is a not a {3}; it is a ") +
                   ("{4}")).format(caller, varname, vardata, datatype, type(vardata))
        print(err_str)
        raise ValueError(err_str)


################################################################################

def strclass(cls: Type[Any]) -> str:
    """ Return the classname of a class"""
    return "%s" % (cls.__module__)

def strqual(cls: Type[Any]) -> str:
    """ Return the qualname of a class"""
    return "%s" % (cls.__qualname__)
