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

""" This module provides the a large number of support functions available
    for tests within a SSTTestCase.
"""

import configparser
import difflib
import multiprocessing
import os
import platform
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tarfile
import threading
import time
import traceback
import unittest
from pathlib import Path
from shutil import which
from typing import (Any, Callable, List, Mapping, Optional, Sequence, Tuple,
                    Type, TypeVar, Union)
from warnings import warn

import test_engine_globals
from test_engine_support import check_param_type

if not sys.warnoptions:
    import os
    import warnings
    warnings.simplefilter("once") # Change the filter in this process
    os.environ["PYTHONWARNINGS"] = "once" # Also affect subprocesses

################################################################################

OS_DIST_OSX = "OSX"
OS_DIST_ARCH = "ARCH"
OS_DIST_CENTOS = "CENTOS"
OS_DIST_RHEL = "RHEL"
OS_DIST_TOSS = "TOSS"
OS_DIST_UBUNTU = "UBUNTU"
OS_DIST_ROCKY = "ROCKY"
OS_DIST_UNDEF = "UNDEFINED"

################################################################################

pin_exec_path = ""

################################################################################

class SSTTestCaseException(Exception):
    """ Generic Exception support for SSTTestCase
    """
    def __init__(self, value: Union[Exception, str]) -> None:
        super(SSTTestCaseException, self).__init__(value)
        self.value = value
    def __str__(self) -> str:
        return repr(self.value)

################################################################################

################################################################################
# Commandline Information Functions
################################################################################

def testing_check_is_nightly() -> bool:
    """If Nightly is in the BUILD_TAG it's very likely a Nightly build."""
    build_tag = os.getenv("BUILD_TAG")
    if build_tag is not None and "Nightly" in build_tag:
        return True
    return False

###

def testing_check_is_in_debug_mode() -> bool:
    """ Identify if test frameworks is in debug mode

        Returns:
            (bool) True if test frameworks is in debug mode
    """
    return test_engine_globals.TESTENGINE_DEBUGMODE

###

def testing_check_is_in_log_failures_mode() -> bool:
    """ Identify if test frameworks is in log failures mode

        Returns:
            (bool) True if test frameworks is in log failures mode
    """
    return test_engine_globals.TESTENGINE_LOGFAILMODE

###

def testing_check_is_in_concurrent_mode() -> bool:
    """ Identify if test frameworks is in concurrent mode

        Returns:
            (bool) True if test frameworks is in concurrent mode
    """
    return test_engine_globals.TESTENGINE_CONCURRENTMODE

###

def testing_check_get_num_ranks() -> int:
    """ Get the number of ranks defined to be run during testing

        Returns:
            (int) The number of requested run ranks
    """
    return test_engine_globals.TESTENGINE_SSTRUN_NUMRANKS

###

def testing_check_get_num_threads() -> int:
    """ Get the number of threads defined to be run during testing

        Returns:
            (int) The number of requested run threads
    """
    return test_engine_globals.TESTENGINE_SSTRUN_NUMTHREADS

################################################################################
# PIN Information Functions
################################################################################

def testing_is_PIN_loaded() -> bool:
    # Look to see if PIN is available
    pindir_found = False
    pin_path = os.environ.get('INTEL_PIN_DIRECTORY')
    if pin_path is not None:
        pindir_found = os.path.isdir(pin_path)
    return pindir_found

def testing_is_PIN_Compiled() -> bool:
    global pin_exec_path
    pin_exec = sst_elements_config_include_file_get_value(define="PINTOOL_EXECUTABLE", type=str, default="", disable_warning=True)
    pin_exec_path = pin_exec
    return pin_exec != ""

def testing_is_PIN2_used() -> bool:
    warn("testing_is_PIN2_used() is deprecated and will be removed in future versions of SST.",
        DeprecationWarning, stacklevel=2)

    global pin_exec_path
    if testing_is_PIN_Compiled():
        return "/pin.sh" in pin_exec_path
    else:
        return False

def testing_is_PIN3_used() -> bool:
    global pin_exec_path
    if testing_is_PIN_Compiled():
        if testing_is_PIN2_used():
            return False
        else:
            # Make sure pin is at the end of the string
            pinstr = "/pin"
            idx = pin_exec_path.rfind(pinstr)
            if idx == -1:
                return False
            else:
                found_pin = (idx + len(pinstr)) == len(pin_exec_path)
                return found_pin
    else:
        return False

################################################################################
# System Information Functions
################################################################################

def host_os_get_system_node_name() -> str:
    """ Get the node name of the system

        Returns:
            (str) Returns the system node name
    """
    warn("host_os_get_system_node_name() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return platform.node()

###

def host_os_get_kernel_type() -> str:
    """ Get the Kernel Type

        Returns:
            (str) 'Linux' or 'Darwin' as the Kernel Type
    """
    warn("host_os_get_kernel_type() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return platform.system()

def host_os_get_kernel_release() -> str:
    """ Get the Kernel Release number

        Returns:
            (str) Kernel Release Number.  Note: This is not the same as OS version
    """
    warn("host_os_get_kernel_release() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return platform.release()

def host_os_get_kernel_arch() -> str:
    """ Get the Kernel System Arch

        Returns:
            (str) 'x86_64' on Linux; 'i386' on OSX as the Kernel Architecture
    """
    warn("host_os_get_kernel_arch() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return platform.machine()

def host_os_get_distribution_type() -> str:
    """ Get the os distribution type

        Returns:
            (str)
            'OSX' for Mac OSX;
            'CENTOS' CentOS;
            'RHEL' for Red Hat Enterprise Linux;
            'TOSS' for Toss;
            'UBUNTU' for Ubuntu;
            'ROCKY' for Rocky;
            'UNDEFINED' an undefined OS.
    """
    k_type = platform.system()
    if k_type == 'Linux':
        lin_dist = _get_linux_distribution()
        dist_name = lin_dist[0].lower()
        if "arch" in dist_name:
            return OS_DIST_ARCH
        if "centos" in dist_name:
            return OS_DIST_CENTOS
        if "red hat" in dist_name:
            return OS_DIST_RHEL
        if "toss" in dist_name:
            return OS_DIST_TOSS
        if "ubuntu" in dist_name:
            return OS_DIST_UBUNTU
        if "rocky" in dist_name:
            return OS_DIST_ROCKY
    elif k_type == 'Darwin':
        return OS_DIST_OSX
    return OS_DIST_UNDEF

def host_os_get_distribution_version() -> str:
    """ Get the os distribution version

        Returns:
            (str) The OS distribution version
    """
    k_type = platform.system()
    if k_type == 'Linux':
        lin_dist = _get_linux_distribution()
        return lin_dist[1]
    if k_type == 'Darwin':
        mac_ver = platform.mac_ver()
        return mac_ver[0]
    return "Undefined"

###

def host_os_is_osx() -> bool:
    """ Check if OS distribution is OSX

        Returns:
            (bool) True if OS Distribution is OSX
    """
    warn("host_os_is_osx() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return host_os_get_distribution_type() == OS_DIST_OSX

def host_os_is_linux() -> bool:
    """ Check if OS distribution is Linux

        Returns:
            (bool) True if OS Distribution is Linux
    """
    warn("host_os_is_linux() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return not host_os_get_distribution_type() == OS_DIST_OSX

def host_os_is_centos() -> bool:
    """ Check if OS distribution is CentOS

        Returns:
            (bool) True if OS Distribution is CentOS
    """
    warn("host_os_is_centos() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return host_os_get_distribution_type() == OS_DIST_CENTOS

def host_os_is_rhel() -> bool:
    """ Check if OS distribution is RHEL

        Returns:
            (bool) True if OS Distribution is RHEL
    """
    warn("host_os_is_rhel() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return host_os_get_distribution_type() == OS_DIST_RHEL

def host_os_is_toss() -> bool:
    """ Check if OS distribution is Toss

        Returns:
            (bool) True if OS Distribution is Toss
    """
    warn("host_os_is_toss() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return host_os_get_distribution_type() == OS_DIST_TOSS

def host_os_is_ubuntu()-> bool:
    """ Check if OS distribution is Ubuntu

        Returns:
            (bool) True if OS Distribution is Ubuntu
    """
    warn("host_os_is_ubuntu() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return host_os_get_distribution_type() == OS_DIST_UBUNTU

def host_os_is_rocky() -> bool:
    """ Check if OS distribution is Rocky

        Returns:
            (bool) True if OS Distribution is Rocky
    """
    warn("host_os_is_rocky() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    return host_os_get_distribution_type() == OS_DIST_ROCKY


###

def host_os_get_num_cores_on_system() -> int:
    """ Get number of cores on the system

        Returns:
            (int) Number of cores on the system
    """
    warn("host_os_get_num_cores_on_system() is deprecated and will be removed in future versions of SST.",
         DeprecationWarning, stacklevel=2)
    num_cores = multiprocessing.cpu_count()
    return num_cores

################################################################################
# SST Skipping Support
################################################################################

def _testing_check_is_scenario_filtering_enabled(scenario_name: str) -> bool:
    """ Determine if a scenario filter name is enabled

        Args:
            scenario_name (str): The scenario filter name to check

        Returns:
           (bool) True if the scenario filter name is enabled
    """
    check_param_type("scenario_name", scenario_name, str)
    return scenario_name in test_engine_globals.TESTENGINE_SCENARIOSLIST

###

# Used for unittest.skip decorator; taken from
# https://github.com/python/typeshed/blob/8cdc1c141b0b9fb617da87319b23206151ab9954/stdlib/unittest/case.pyi#L35
_FT = TypeVar("_FT", bound=Callable[..., Any])

def skip_on_scenario(scenario_name: str, reason: str) -> Callable[[_FT], _FT]:
    """ Skip a test if a scenario filter name is enabled

        Args:
            scenario_name (str): The scenario filter name to check
            reason (str): The reason for the skip
    """
    check_param_type("scenario_name", scenario_name, str)
    check_param_type("reason", reason, str)
    if not _testing_check_is_scenario_filtering_enabled(scenario_name):
        return lambda func: func
    return unittest.skip(reason)

###

def skip_on_sstsimulator_conf_empty_str(section: str, key: str, reason: str) -> Callable[[_FT], _FT]:
    """ Skip a test if a section/key in the sstsimulator.conf file is missing an
        entry

        Args:
            section (str): The section in the sstsimulator.conf to check
            key (str): The key in the sstsimulator.conf to check
            reason (str): The reason for the skip
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    check_param_type("reason", reason, str)
    rtn_str = sstsimulator_conf_get_value(section, key, str, "")
    if rtn_str != "":
        return lambda func: func
    return unittest.skip(reason)

################################################################################
# SST Core Configuration include file (sst_config.h.conf) Access Functions
################################################################################

def sst_core_config_include_file_get_value_int(
    define: str,
    default: Optional[int] = None,
    disable_warning: bool = False,
) -> int:
    """ Retrieve a define from the SST Core Configuration Include File (sst_config.h)

        Args:
            define (str): The define to look for
            default (int): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (int) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    warn("sst_core_config_include_file_get_value_int() is deprecated and will be removed in future versions of SST. \
         Use sst_core_config_include_file_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sst_config_include_file_value(
        include_dict=test_engine_globals.TESTENGINE_CORE_CONFINCLUDE_DICT,
        include_source="sst_config.h",
        define=define,
        default=default,
        data_type=int,
        disable_warning=disable_warning,
    )

###

def sst_core_config_include_file_get_value_str(
    define: str,
    default: Optional[str] = None,
    disable_warning: bool = False,
) -> str:
    """ Retrieve a define from the SST Core Configuration Include File (sst_config.h)

        Args:
            define (str): The define to look for
            default (str): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (str) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    warn("sst_core_config_include_file_get_value_str() is deprecated and will be removed in future versions of SST. \
         Use sst_core_config_include_file_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sst_config_include_file_value(
        include_dict=test_engine_globals.TESTENGINE_CORE_CONFINCLUDE_DICT,
        include_source="sst_config.h",
        define=define,
        default=default,
        data_type=str,
        disable_warning=disable_warning,
    )

###

T_include = TypeVar("T_include", str, int)


def sst_core_config_include_file_get_value(
    define: str,
    type: Type[T_include],
    default: Optional[T_include] = None,
    disable_warning: bool = False,
) -> T_include:
    """Retrieve a define from the SST Core Configuration Include File (sst_config.h)

    Args:
        define (str): The define to look for
        type (Type): The expected type of the return value
        default (optional): Default Return if failure occurs
        disable_warning (bool): Disable the warning if define not found

    Returns:
        Value for specified define
    """
    return _get_sst_config_include_file_value(
        include_dict=test_engine_globals.TESTENGINE_CORE_CONFINCLUDE_DICT,
        include_source="sst_config.h",
        define=define,
        default=default,
        data_type=type,
        disable_warning=disable_warning,
    )


################################################################################
# SST Elements Configuration include file (sst_element_config.h.conf) Access Functions
################################################################################

def sst_elements_config_include_file_get_value_int(
    define: str,
    default: Optional[int] = None,
    disable_warning: bool = False,
) -> int:
    """ Retrieve a define from the SST Elements Configuration Include File (sst_element_config.h)

        Args:
            define (str): The define to look for
            default (int): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (int) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    warn("sst_elements_config_include_file_get_value_int() is deprecated and will be removed in future versions of SST. \
         Use sst_elements_config_include_file_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sst_config_include_file_value(
        include_dict=test_engine_globals.TESTENGINE_ELEM_CONFINCLUDE_DICT,
        include_source="sst_element_config.h",
        define=define,
        default=default,
        data_type=int,
        disable_warning=disable_warning,
    )

###

def sst_elements_config_include_file_get_value_str(
    define: str,
    default: Optional[str] = None,
    disable_warning: bool = False,
) -> str:
    """ Retrieve a define from the SST Elements Configuration Include File (sst_element_config.h)

        Args:
            define (str): The define to look for
            default (str): Default Return if failure occurs
            disable_warning (bool): Disable logging the warning if define is not found

        Returns:
            (str) The returned data or default if not found in the include file

        Raises:
            SSTTestCaseException: if type is incorrect OR no data AND default
                                  is not provided
    """
    warn("sst_elements_config_include_file_get_value_str() is deprecated and will be removed in future versions of SST. \
         Use sst_elements_config_include_file_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sst_config_include_file_value(
        include_dict=test_engine_globals.TESTENGINE_ELEM_CONFINCLUDE_DICT,
        include_source="sst_element_config.h",
        define=define,
        default=default,
        data_type=str,
        disable_warning=disable_warning,
    )

###

def sst_elements_config_include_file_get_value(
    define: str,
    type: Type[T_include],
    default: Optional[T_include] = None,
    disable_warning: bool = False,
) -> T_include:
    """Retrieve a define from the SST Elements Configuration Include File (sst_element_config.h)

    Args:
        define (str): The define to look for
        type (Type): The expected type of the return value
        default (optional): Default Return if failure occurs
        disable_warning (bool): Disable the warning if define not found

    Returns:
        Value for specified define
    """
    return _get_sst_config_include_file_value(
        include_dict=test_engine_globals.TESTENGINE_ELEM_CONFINCLUDE_DICT,
        include_source="sst_element_config.h",
        define=define,
        default=default,
        data_type=type,
        disable_warning=disable_warning,
    )


################################################################################
# SST Configuration file (sstsimulator.conf) Access Functions
################################################################################

def sstsimulator_conf_get_value_str(section: str, key: str, default: Optional[str] = None) -> str:
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (str): Default Return if failure occurs

        Returns:
            (str) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    warn("sstsimulator_conf_get_value_str() is deprecated and will be removed in future versions of SST. \
         Use sstsimulator_conf_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sstsimulator_conf_value(section=section, key=key, default=default, data_type=str)

###

def sstsimulator_conf_get_value_int(section: str, key: str, default: Optional[int] = None) -> int:
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (int): Default Return if failure occurs

        Returns:
            (int) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    warn("sstsimulator_conf_get_value_int() is deprecated and will be removed in future versions of SST. \
         Use sstsimulator_conf_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sstsimulator_conf_value(section=section, key=key, default=default, data_type=int)

###

def sstsimulator_conf_get_value_float(
    section: str, key: str, default: Optional[float] = None
) -> float:
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (float): Default Return if failure occurs

        Returns:
            (float) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    warn("sstsimulator_conf_get_value_float() is deprecated and will be removed in future versions of SST. \
         Use sstsimulator_conf_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sstsimulator_conf_value(section=section, key=key, default=default, data_type=float)

###

def sstsimulator_conf_get_value_bool(
    section: str, key: str, default: Optional[bool] = None
) -> bool:
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)

        NOTE: "1", "yes", "true", and "on" will return True;
              "0", "no", "false", and "off" willl return False

        Args:
            section (str): The [section] to look for the key
            key (str): The key to find
            default (bool): Default Return if failure occurs

        Returns:
            (bool) The returned data or default if not found in the config file

        Raises:
            SSTTestCaseException: if no data AND default is not provided
    """
    warn("sstsimulator_conf_get_value_bool() is deprecated and will be removed in future versions of SST. \
         Use sstsimulator_conf_get_value() instead.", DeprecationWarning, stacklevel=2)
    return _get_sstsimulator_conf_value(section=section, key=key, default=default, data_type=bool)

###

T_conf = TypeVar("T_conf", str, int, float, bool)


def sstsimulator_conf_get_value(
    section: str, key: str, type: Type[T_conf], default: Optional[T_conf] = None
) -> T_conf:
    """Get the configuration value from the SST Configuration File (sstsimulator.conf)

    Args:
        section (str): The [section] to look for the key
        key (str): The key to find
        type (Type): The expected type of the return value
        default (optional): Default Return if failure occurs

    Returns:
        Value for section[key]
    """
    return _get_sstsimulator_conf_value(section=section, key=key, default=default, data_type=type)

###

def sstsimulator_conf_get_sections() -> List[str]:
    """ Retrieve a list of sections that exist in the SST Configuration File (sstsimulator.conf)

        Returns:
            (list of str) The list of sections in the file

        Raises:
            SSTTestCaseException: If an error occurs
    """
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.sections()
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_get_section_keys(section: str) -> List[str]:
    """ Retrieve a list of keys under a section that exist in the
        SST Configuration File  (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the list of keys

        Returns:
            (list of str) The list of keys under a section in the file

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.options(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_get_all_keys_values_from_section(section: str) -> List[Tuple[str, str]]:
    """ Retrieve a list of tuples that contain all the key - value pairs
        under a section that exists in the SST Configuration File (sstsimulator.conf)

        Args:
            section (str): The [section] to look for the key

        Returns:
            (list of tuples of str) The list of tuples of key - value pairs

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.items(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_does_have_section(section: str) -> bool:
    """ Check if the SST Configuration File (sstsimulator.conf) has a defined
        section

        Args:
            section (str): The [section] to look for

        Returns:
            (bool) True if the [section] is found

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.has_section(section)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

###

def sstsimulator_conf_does_have_key(section: str, key: str) -> bool:
    """ Check if the SST Configuration File (sstsimulator.conf) has a defined key
        within a section
        Args:
            section (str): The [section] within to search for the key
            key (str): The key to search for

        Returns:
            (bool) True if the key exists within the [section]

        Raises:
            SSTTestCaseException: If an error occurs
    """
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        return core_conf_file_parser.has_option(section, key)
    except configparser.Error as exc_e:
        raise SSTTestCaseException(exc_e)

################################################################################
# Logging Functions
################################################################################

def log(logstr: str) -> None:
    """ Log a general message.

        This will not output unless we are outputing in >= normal mode.

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    if test_engine_globals.TESTENGINE_VERBOSITY >= test_engine_globals.VERBOSE_NORMAL:
        log_forced(logstr)

###

def log_forced(logstr: str) -> None:
    """ Log a general message, no matter what the verbosity is.

        if in the middle of testing, it will precede with a '\\n' to slip
        in-between unittest outputs

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    extra_lf = ""
    if test_engine_globals.TESTRUN_TESTRUNNINGFLAG:
        extra_lf = "\n"
    print(("{0}{1}".format(extra_lf, logstr)))

###

def log_debug(logstr: str) -> None:
    """ Log a 'DEBUG:' message.

        Log will only happen if in debug verbosity mode.

        Args:
            logstr (str): string to be logged
    """
    if test_engine_globals.TESTENGINE_DEBUGMODE:
        finalstr = "DEBUG: {0}".format(logstr)
        log_forced(finalstr)

###

def log_failure(logstr: str) -> None:
    """ Log a test failure.

        Log will only happen if in log failure mode.
        NOTE: Testcases may call log_failure to log any test failure data.  Log
              Log output will only occur if log failure mode is enabled.
              The intent of this function is to allow tests to log failure info
              (useful for troubleshooting) during a run if the 'log failure mode'
              is enabled.

        Args:
            logstr (str): string to be logged
    """
    if test_engine_globals.TESTENGINE_LOGFAILMODE:
        finalstr = "{0}".format(logstr)
        log_forced(finalstr)

###

def log_info(logstr: str, forced: bool = True) -> None:
    """ Log a 'INFO:' message.

        Args:
            logstr (str): string to be logged
            forced (bool): If true Always force the logging regardless of verbosity;
                           otherwise, perform a normal log.
    """
    check_param_type("logstr", logstr, str)
    finalstr = "INFO: {0}".format(logstr)
    if forced:
        log_forced(finalstr)
    else:
        log(finalstr)
###

def log_error(logstr: str) -> None:
    """ Log a 'ERROR:' message.

        Log will occur no matter what the verbosity is

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    finalstr = "ERROR: {0}".format(logstr)
    log_forced(finalstr)
    test_engine_globals.TESTENGINE_ERRORCOUNT += 1

###

def log_warning(logstr: str) -> None:
    """ Log a 'WARNING:' message.

        Log will occur no matter what the verbosity is

        Args:
            logstr (str): string to be logged
    """
    check_param_type("logstr", logstr, str)
    finalstr = "WARNING: {0}".format(logstr)
    log_forced(finalstr)

###

def log_fatal(errstr: str) -> None:
    """ Log a 'FATAL:' message.

        Log will occur no matter what the verbosity is and
        THIS WILL KILL THE TEST ENGINE AND RETURN FAILURE

        Args:
            errstr (str): string to be logged
    """
    check_param_type("errstr", errstr, str)
    finalstr = "FATAL: {0}".format(errstr)
    log_forced(finalstr)
    sys.exit(2)

###

def log_testing_note(note_str: str) -> None:
    """ Log a testing note

        Add a testing note that will be displayed at the end of the test run
        in the results section.  These notes can be used for adding any
        info/notes/observations that is desired to be reviewed by the user at the
        end of the test run.  Adding a note has no impact on the Pass/Fail status
        of any test.  Also, if debug mode is enabled, the note will be logged
        to the console.  The text "NOTE : " will be prepended to the front of the
        note_str.

        Args:
            note_str (str): string to be added to notes list
    """
    check_param_type("note_str", note_str, str)
    final_note = "NOTE: {0}".format(note_str)
    test_engine_globals.TESTENGINE_TESTNOTESLIST.append(final_note)
    log_debug(final_note)

################################################################################
### Testing Directories
################################################################################

def test_output_get_run_dir() -> str:
    """ Return the path of the output run directory

        Returns:
            (str) Path to the output run directory
    """
    return test_engine_globals.TESTOUTPUT_RUNDIRPATH

###

def test_output_get_tmp_dir() -> str:
    """ Return the path of the output tmp directory

        Returns:
            (str) Path to the output tmp directory
    """
    return test_engine_globals.TESTOUTPUT_TMPDIRPATH

################################################################################
### Testing Support
################################################################################
def combine_per_rank_files(
    filename: str, header_lines_to_remove: int = 0, remove_header_from_first_file: bool = False
) -> None:
    """Combines per rank files into a single file.

    This assumes that filename will be the base name with format
    base.ext and that the per rank files will be of the format
    base_0.ext, base_1.ext, etc.  The files will be concatenated in
    order and written to the base filename.

        Args:
            filename (str): base name of file

            header_lines_to_remove (int): number of header lines to remove from files

            remove_header_from_first_file (bool): controls whether
            header lines are removed from first file or no

        Returns:
            No return value

    """
    check_param_type("filename", filename, str)
    check_param_type("header_lines_to_remove", header_lines_to_remove, int)
    check_param_type("remove_header_from_first_file", remove_header_from_first_file, bool)

    # Get the number of MPI ranks
    ranks = testing_check_get_num_ranks()

    # Nothing to be done if there's only one rank
    if ranks == 1:
        return

    # Split the basename and extention from the file name
    basename, extension = os.path.splitext(filename)

    # Output lines
    out_lines = []

    for x in range(ranks):
        # Get full name of file in format base_rank.ext (extension
        # variable includes the .)
        full_name = "{0}_{1}{2}".format(basename, x, extension)

        # Check to make sure file exists
        if not os.path.isfile(full_name):
            log_error("Cannot combine rank per file files: File {0} does not exist".format(full_name))
            return

        # Read in the output file
        with open(full_name) as fp:
            count = 0
            # If this is the first file and we don't remove header
            # lines, adjust count so that nothing is removed
            if x == 0 and not remove_header_from_first_file:
                count = header_lines_to_remove
            for line in fp:
                if count < header_lines_to_remove:
                    count += 1
                    continue
                out_lines.append(line)
                count += 1

    # write the output file
    with open(filename, "w") as fp:
        for line in out_lines:
            fp.write(line)


def testing_parse_stat(line: str) -> Optional[List[Union[str, int, float]]]:
    """ Return a parsed statistic or 'None' if the line does not match a known statistic format

        This function will recognize an Accumulator statistic in statOutputConsole format that is generated by
        SST at the end of simulation.

        Args:
            line (str): string to parse into a statistic

        Returns:
            (list) list of [component, sum, sumSq, count, min, max] or 'None' if not a statistic
    """
    cons_accum = re.compile(r' ([\w.]+)\.(\w+) : Accumulator : Sum.(\w+) = ([\d.]+); SumSQ.\w+ = ([\d.]+); Count.\w+ = ([\d.]+); Min.\w+ = ([\d.]+); Max.\w+ = ([\d.]+);')
    m = cons_accum.match(line)
    if m is None:
        return None

    stat = [m.group(1), m.group(2)]
    if 'u' in m.group(3) or 'i' in m.group(3):
        stat.append(int(m.group(4)))
        stat.append(int(m.group(5)))
        stat.append(int(m.group(6)))
        stat.append(int(m.group(7)))
        stat.append(int(m.group(8)))
    elif 'f' in m.group(3):
        stat.append(float(m.group(4)))
        stat.append(float(m.group(5)))
        stat.append(float(m.group(6)))
        stat.append(float(m.group(7)))
        stat.append(float(m.group(8)))
    else:
        return None
    return stat

def testing_stat_output_diff(
    outfile: str,
    reffile: str,
    ignore_lines: List[str] = [],
    tol_stats: Mapping[str, float] = {},
    new_stats: bool = False,
) -> Tuple[bool, List[List[Union[str, int, float]]], List[List[str]]]:
    """ Perform a diff of statistic outputs with special handling based on arguments
        This diff is not sensitive to line ordering

        Limitations:
        Because the test framework captures console output, the statistic format is assumed to be Console.
        The statistic parser does not recognize non-Accumulator statistics.
        This function has been tested against statistics generated at the end of simulation and does not handle diffing periodic statistics by timestamp.

        Args:
            outfile (str): filename of the output file to check
            reffile (str): filename of the reference file to diff against
            ignore_lines (list): list of strings to ignore in both the ref and output files.
                                 Any line that contains one of these strings will be ignored.
            tol_stats (dictionary): Dictionary mapping a statistic name to a list of tolerances on each field (sum, sumSq, count, min, max).
                                    A tolerance of 'X' indicates don't care. All values are treated as a +/- on the reference value.
            new_stats (bool): If true, the diff will ignore any statistics that are present in the outfile but not present in the reffile

        Returns:
           (bool) True if the diff check passed
           (list) A list of statistics that diffed with '<' indicating reffile lines and '>' indicating outfile lines
           (list) A list of non-statistic lines that diffed with '<' indicating reffile lines and '>' indicating outfile lines
    """
    # Parse files, ignoring lines in 'ignore_lines'
    ref_lines = []
    ref_stats = []
    out_lines = []
    out_stats = []
    with open(reffile, 'r') as fp:
        lines = fp.read().splitlines()
        for line in lines:
            if not any(x in line for x in ignore_lines):
                stat = testing_parse_stat(line)
                if stat is not None:
                    ref_stats.append(stat)
                else:
                    ref_lines.append(line)

    # Parse output file, filtering in line
    with open(outfile, 'r') as fp:
        lines = fp.read().splitlines()
        for line in lines:
            if not any(x in line for x in ignore_lines):
                stat = testing_parse_stat(line)
                if stat is None:  # Not a statistic
                    if line not in ref_lines:
                        out_lines.append(line)
                    else:
                        ref_lines.remove(line)
                    continue

                # Filter exact match to reference
                if stat in ref_stats:
                    ref_stats.remove(stat)
                    continue

                # Filter new statistics
                if new_stats and not any((row[0] == stat[0] and row[1] == stat[1]) for row in ref_stats):
                    continue

                # Filter on tolerance if possible
                if stat[1] in tol_stats:
                    found = False
                    for s in ref_stats:
                        if s[0] == stat[0] and s[1] == stat[1]:
                            rstat = s
                            found = True
                            break

                    if not found:
                        out_stats.append(stat)
                        continue

                    diffs = False
                    tol = tol_stats[stat[1]]  # type: ignore [index]
                    for i, t in enumerate(tol):  # type: ignore [arg-type, var-annotated]
                        if t != 'X' and ((rstat[2+i] - t) > stat[2+i] or (rstat[2+i] + t) < stat[2+i]):
                            diffs = True
                            break

                    if diffs:
                        out_stats.append(stat)
                    else:
                        ref_stats.remove(rstat)
                    continue

                # Not filtered
                out_stats.append(stat)

    # Combine diffs
    stat_diffs = [ ['<',x[0],x[1],x[2],x[3],x[4],x[5],x[6]] for x in ref_stats ]
    stat_diffs += [ ['>',x[0],x[1],x[2],x[3],x[4],x[5],x[6]] for x in out_stats ]

    line_diffs = [ ['<',x] for x in ref_lines ]
    line_diffs += [ ['>',x] for x in out_lines ]

    if len(stat_diffs) > 0 or len(line_diffs) > 0:
        return False, stat_diffs, line_diffs
    else:
        return True, stat_diffs, line_diffs

###

### Built in LineFilters for filtering diffs
class LineFilter:
    def __init__(self) -> None:
        self.apply_to_ref_file = True
        self.apply_to_out_file = True

    """ Base class for filtering lines in diffs

        Args:
            line (str): Line to check

        Returns:
            (bool) Filtered line or None if line should be removed
    """
    def filter(self, line: str) -> Optional[str]:
        pass

    def reset(self) -> None:
        pass


class StartsWithFilter(LineFilter):
    """ Filters out any line that starts with a specified string
    """
    def __init__(self, prefix: str) -> None:
        super().__init__()
        self._prefix = prefix;

    def filter(self, line: str) -> Optional[str]:
        """ Checks to see if the line starts with the prefix specified in constructor

            Args:
                line (str): Line to check

            Returns:
                line if line does not start with the prefix and None if it does
        """
        if line.startswith(self._prefix):
            return None
        return line

class IgnoreAllAfterFilter(LineFilter):
    """ Filters out any line that starts with a specified string and all lines after it
    """
    def __init__(self, prefix: str, keep_line: bool = False) -> None:
        super().__init__()
        self._prefix = prefix;
        self._keep_line = keep_line
        self._found = False

    def reset(self) -> None:
        self._found = False

    def filter(self, line: str) -> Optional[str]:
        """ Checks to see if the line starts with the prefix specified in constructor

            Args:
                line (str): Line to check

            Returns:
                line if line does not start with the prefix and None if it does
        """
        if self._found: return None
        if line.startswith(self._prefix):
            self._found = True
            if self._keep_line:
                return line
            return None
        return line


class IgnoreAllBeforeFilter(LineFilter):
    """ Filters out any line that starts with a specified string and all lines before it
    """
    def __init__(self, prefix: str, keep_line: bool = False, times: int = 1) -> None:
        super().__init__()
        self._prefix = prefix;
        self._keep_line = keep_line
        self._times = times
        self._count = times
        self._found = False

    def reset(self) -> None:
        self._found = False
        self._count = self._times

    def filter(self, line: str) -> Optional[str]:
        """ Checks to see if the line starts with the prefix specified in constructor

            Args:
                line (str): Line to check

            Returns:
                line if line does not start with the prefix and None if it does
        """
        if self._found: return line
        if line.startswith(self._prefix):
            self._count -= 1
            if self._count == 0:
                self._found = True
                if self._keep_line:
                    return line
                return None
        return None



class IgnoreWhiteSpaceFilter(LineFilter):
    """Converts any stream of whitespace (space or tabs) to a single
    space.  Newlines are not filtered.

    """
    def __init__(self) -> None:
        super().__init__()
        pass

    def filter(self, line: str) -> Optional[str]:
        """ Converts any stream of whitespace to a single space.

            Args:
                line (str): Line to filter

            Returns:
                filtered line
        """
        filtered_line = str()
        ws_stream = False
        for x in line:
            if x == ' ' or x == '\t':
                if not ws_stream:
                    filtered_line += ' '
                    ws_stream = True
            else:
                filtered_line += x
                ws_stream = False

        return filtered_line

class RemoveRegexFromLineFilter(LineFilter):
    """Filters out portions of line that match the specified regular expression
    """
    def __init__(self, expr: str) -> None:
        super().__init__()
        self.regex = expr

    def filter(self, line: str) -> Optional[str]:
        """ Removes all text before the specified prefix.

            Args:
                line (str): Line to check

            Returns:
                text from line minus any text that matched the regular expression
        """
        mtch = re.search(self.regex, line)
        while mtch:
            line = line[: mtch.start()] + line[mtch.end() :]
            mtch = re.search(self.regex, line)

        return line

class CheckpointInfoFilter(LineFilter):
    """Filters out the two info messages generated by checkpointing
    """
    def __init__(self) -> None:
        super().__init__()

    def filter(self, line: str) -> Optional[str]:
        """ Checks to see if the line starts with the prefix specified in constructor

            Args:
                line (str): Line to check

            Returns:
                line if line does not start with the prefix and None if it does
        """
        if line.startswith("# Simulation Checkpoint:"):
            return None
        if line.startswith("# Creating simulation checkpoint"):
            return None
        return line


class CheckpointRefFileFilter(IgnoreAllBeforeFilter):
    """ Filters out any line that starts with a specified string and all lines before it
    """
    def __init__(self, times: int = 1) -> None:
        super().__init__("# Simulation Checkpoint:", False, times)
        self.apply_to_out_file = False



def _read_and_filter(fileloc: str, filters: Sequence[LineFilter], sort: bool, is_ref: bool) -> List[str]:
    lines = list()

    with open(fileloc) as fp:
        for filter in filters:
            filter.reset()
        for line in fp:
            filt_line = line
            for filter in filters:
                if (is_ref and filter.apply_to_ref_file) or (not is_ref and filter.apply_to_out_file):
                     filt_line = filter.filter(filt_line)  # type: ignore [assignment]
                     if not filt_line:
                         break

            if filt_line:
                lines.append(filt_line)

    if sort:
        lines.sort()

    return lines


### Diff functions
def testing_compare_filtered_diff(
    test_name: str,
    outfile: str,
    reffile: str,
    sort: bool = False,
    filters: Union[LineFilter, List[LineFilter]] = list(),
    do_statistics_comparison: bool = False,
) -> bool:
    """Filter, optionally sort and then compare 2 files for a difference.

        Args:
            test_name (str): Unique name to prefix the diff file and 2 sorted files.
            outfile (str): Path to the output file
            reffile (str): Path to the reference file
            sort (bool): If True, lines from files will be sorted before diffing
            filters (list): List of filters to apply to the lines of
              the output and reference files.  Filters will be applied
              in order, but will break early if any filter returns None

        Returns:
            (bool) True if the 2 sorted files match

    """

    check_param_type("test_name", test_name, str)
    check_param_type("outfile", outfile, str)
    check_param_type("reffile", reffile, str)
    if isinstance(filters, LineFilter):
        filters = [filters]
    check_param_type("filters", filters, list)

    if not os.path.isfile(outfile):
        log_error("Cannot diff files: Out File {0} does not exist".format(outfile))
        return False

    if not os.path.isfile(reffile):
        log_error("Cannot diff files: Ref File {0} does not exist".format(reffile))
        return False

    # Read in the output and reference files, and optionally sorting them
    out_lines = _read_and_filter(outfile, filters, sort, False)
    ref_lines = _read_and_filter(reffile, filters, sort, True)

    # Get the diff between the files
    diff = difflib.unified_diff(out_lines,ref_lines,outfile,reffile,n=1)

    # Write the diff to a file and count the number of lines of output
    # to determine if they matched or not (the output of the diff is a
    # generator, so the only way to see if it's empty is to start to
    # write the data).
    diff_file = "{1}/{0}_diff_file".format(test_name, test_output_get_tmp_dir())
    count = 0
    with open(diff_file, "w") as fp:
        for line in diff:
            fp.write(line)
            count += 1

    # Files are the same if there were no lines of diff output
    return count == 0

###


def testing_compare_diff(
    test_name: str, outfile: str, reffile: str, ignore_ws: bool = False
) -> bool:
    """ compare 2 files for a diff.

        Args:
            test_name (str): Unique name to prefix the diff file.
            outfile (str): Path to the output file.
            reffile (str): Path to the reference file.
            ignore_ws (bool): ignore whitespace during the compare [False].

        Returns:
            (bool) True if the 2 files match.
    """
    check_param_type("test_name", test_name, str)
    check_param_type("outfile", outfile, str)
    check_param_type("reffile", reffile, str)
    check_param_type("ignore_ws", ignore_ws, bool)

    if ignore_ws:
        return testing_compare_filtered_diff(test_name, outfile, reffile, False, [IgnoreWhiteSpaceFilter()])
    else:
        return testing_compare_filtered_diff(test_name, outfile, reffile, False)

###


def testing_compare_sorted_diff(test_name: str, outfile: str, reffile: str) -> bool:
    """ Sort and then compare 2 files for a difference.

        Args:
            test_name (str): Unique name to prefix the diff file and 2 sorted files.
            outfile (str): Path to the output file
            reffile (str): Path to the reference file

        Returns:
            (bool) True if the 2 sorted files match

    """
    check_param_type("test_name", test_name, str)
    check_param_type("outfile", outfile, str)
    check_param_type("reffile", reffile, str)
    return testing_compare_filtered_diff(test_name, outfile, reffile, True)



###



def testing_compare_filtered_subset(
    outfile: str, reffile: str, filters: Union[LineFilter, List[LineFilter]] = list()
) -> bool:
    """Filter, and then determine if outfile is a subset of reffile

        Args:
            outfile (str): Path to the output file
            reffile (str): Path to the reference file
            filters (list): List of filters to apply to the lines of
              the output and reference files. Filters will be applied
              in order, but will break early if any filter returns None

        Returns:
            (bool) True if outfile is a subset of reffile

    """

    check_param_type("outfile", outfile, str)
    check_param_type("reffile", reffile, str)
    if isinstance(filters, LineFilter):
        filters = [filters]
    check_param_type("filters", filters, list)

    if not os.path.isfile(outfile):
        log_error("Cannot diff files: Out File {0} does not exist".format(outfile))
        return False

    if not os.path.isfile(reffile):
        log_error("Cannot diff files: Ref File {0} does not exist".format(reffile))
        return False

    # Read in the output and reference files and filter them
    out_lines = _read_and_filter(outfile, filters, sort=False, is_ref=False)
    ref_lines = _read_and_filter(reffile, filters, sort=False, is_ref=True)

    # Determine whether subset holds
    return set(out_lines).issubset(set(ref_lines))


###



def testing_get_diff_data(test_name: str) -> str:
    """ Return the diff data file from a regular diff. This should be used
        after a call to testing_compare_sorted_diff(),
        testing_compare_diff() or testing_compare_filtered_diff to
        return the sorted diff data

        Args:
            test_name (str): Unique name used to generate the diff file.

        Returns:
            (str) The diff data file if it exists; otherwise an empty string

    """
    check_param_type("test_name", test_name, str)

    diff_file = "{1}/{0}_diff_file".format(test_name, test_output_get_tmp_dir())

    if not os.path.isfile(diff_file):
        log_debug("diff_file {0} is not found".format(diff_file))
        return ""

    with open(diff_file, 'r') as file:
        data = file.read()

    return data

###


def testing_merge_mpi_files(
    filepath_wildcard: str,
    mpiout_filename: str,
    outputfilepath: str,
    errorfilepath: Optional[str] = None,
) -> None:
    """ Merge a group of common MPI files into an output file
        This works for OpenMPI 4.x and 5.x ONLY

        Args:
            filepath_wildcard (str): The wildcard Path to the files to be merged (OpenMPI 5.x)
            mpiout_filename (str): The name of the MPI output directory (OpenMPI 4.x)
            outputfilepath (str): The output file path for stdout
            errorfilepath (str): The output file path for stderr. If none, stderr redirects to stdout.
    """

    check_param_type("filepath_wildcard", filepath_wildcard, str)
    check_param_type("mpiout_filename", mpiout_filename, str)
    check_param_type("outputfilepath", outputfilepath, str)

    # Delete any output files that might exist
    cmd = "rm -rf {0}".format(outputfilepath)
    os.system(cmd)
    if errorfilepath is not None:
        cmd = "rm -rf {0}".format(errorfilepath)
        os.system(cmd)

    # Check for existing mpiout_filepath (for OpenMPI V4)
    mpipath = "{0}/1".format(mpiout_filename)
    if os.path.isdir(mpipath):
        dirItemList = os.listdir(mpipath)
        for rankdir in dirItemList:
            mpirankoutdir = "{0}/{1}".format(mpipath, rankdir)
            mpioutfile = "{0}/{1}".format(mpirankoutdir, "stdout")
            mpierrfile = "{0}/{1}".format(mpirankoutdir, "stderr")

            if os.path.isdir(mpirankoutdir) and os.path.isfile(mpioutfile):
                cmd = "cat {0} >> {1}".format(mpioutfile, outputfilepath)
                os.system(cmd)
            if os.path.isdir(mpirankoutdir) and os.path.isfile(mpierrfile):
                if errorfilepath is None:
                    cmd = "cat {0} >> {1}".format(mpierrfile, outputfilepath)
                else:
                    cmd = "cat {0} >> {1}".format(mpierrfile, errorfilepath)
                os.system(cmd)
    else:
        # Cat the files together normally (OpenMPI V5)
        # MPI 5.x - name.testfile.prterun-platform-PID@rank.thread.out or .err
        if errorfilepath is None:
            cmd = "cat {0} > {1}".format(filepath_wildcard, outputfilepath)
            os.system(cmd)
        else:
            cmd = "cat {0}.out > {1}".format(filepath_wildcard, outputfilepath)
            os.system(cmd)
            cmd = "cat {0}.err > {1}".format(filepath_wildcard, errorfilepath)
            os.system(cmd)

###

def testing_remove_component_warning_from_file(input_filepath: str) -> None:
    """ Remove SST Component warnings from a file

        This will re-write back to the file with the removed warnings

        Args:
            input_filepath (str): Path to the file to have warnings removed from
    """
    check_param_type("input_filepath", input_filepath, str)

    bad_string = 'WARNING: No components are'
    _remove_lines_with_string_from_file(bad_string, input_filepath)

################################################################################
### OS Basic Or Equivalent Commands
################################################################################

################################################################################

class os_command:
    """ Enables to run subprocess commands in a different thread with a TIMEOUT option.
        This will return a OSCommandResult object.

        Based on a modified version of jcollado's solution:
        http://stackoverflow.com/questions/1191374/subprocess-with-timeout/4825933#4825933
    """
###

    def __init__(
        self,
        cmd_str: str,
        output_file_path: Optional[str] = None,
        error_file_path: Optional[str] = None,
        set_cwd: Optional[str] = None,
        use_shell: bool = False,
    ) -> None:
        """
            Args:
                cmd_str (str): The command to be executed
                output_file_path (str): The file path to send the std output from the cmd
                                        if None send to stdout.
                error_file_path (str): The file path to send the std error from the cmd
                                        if None send to stderr
                set_cwd (str): Path to change dir to before running cmd; if None then
                               use current working directory.
                use_shell (bool): Execute the cmd using the shell (not recommended)
        """
        self._output_file_path = None
        self._error_file_path = None
        self._cmd_str = [""]
        self._process: subprocess.Popen[str] = None  # type: ignore [assignment]
        self._timeout_sec = 60
        # Use an invalid return code rather than None to identify an
        # unintialized value.
        self._run_status_sentinel = -999
        self._run_status = self._run_status_sentinel
        self._run_output = ''
        self._run_error = ''
        self._run_timeout = False
        self._use_shell = use_shell
        self._set_cwd = set_cwd
        self._validate_cmd_str(cmd_str)
        self._output_file_path = self._validate_output_path(output_file_path)
        self._error_file_path = self._validate_output_path(error_file_path)
        self._signal = signal.NSIG
        self._signal_sec = 3
####

    def run(self,
        timeout_sec: int = 60,
        send_signal: int = signal.NSIG,
        signal_sec: int = 3,
        **kwargs: Any,
    ) -> "OSCommandResult":
        """ Run a command then return and OSCmdRtn object.

            Args:
                timeout_sec (int): The maximum runtime in seconds before thread
                                   will be terminated and a timeout error will occur.
                kwargs: Extra parameters e.g., timeout_sec to override the default timeout
        """
        check_param_type("timeout_sec", timeout_sec, int)

        self._timeout_sec = timeout_sec
        self._signal = send_signal
        self._signal_sec = signal_sec

        # Build the thread that will monitor the subprocess with a timeout
        thread = threading.Thread(target=self._run_cmd_in_subprocess, kwargs=kwargs)
        thread.start()
        thread.join(self._timeout_sec)
        if thread.is_alive():
            self._run_timeout = True
            assert self._process is not None
            self._process.kill()
            thread.join()

        # Build a OSCommandResult object to hold the results
        rtn = OSCommandResult(self._cmd_str, self._run_status, self._run_output,
                              self._run_error, self._run_timeout)
        return rtn

####

    def _run_cmd_in_subprocess(self, **kwargs: Any) -> None:
        """ Run the command in a subprocess """
        file_out = None
        file_err = None

        try:
            # Run in either the users chosen directory or the run dir
            if self._set_cwd is None:
                subprocess_path = test_engine_globals.TESTOUTPUT_RUNDIRPATH
            else:
                subprocess_path = os.path.abspath(self._set_cwd)

            # If No output files defined, default stdout and stderr to normal output
            if 'stdout' not in kwargs and self._output_file_path is None:
                kwargs['stdout'] = subprocess.PIPE
            if 'stderr' not in kwargs and self._error_file_path is None:
                kwargs['stderr'] = subprocess.PIPE

            # Create the stderr & stdout to the output files, if stderr path is
            # not defined, then use the normal output file
            if 'stdout' not in kwargs and self._output_file_path is not None:
                file_out = open(self._output_file_path, 'w+')
                kwargs['stdout'] = file_out
                if self._error_file_path is None:
                    kwargs['stderr'] = file_out
            if 'stderr' not in kwargs and self._error_file_path is not None:
                file_err = open(self._error_file_path, 'w+')
                kwargs['stderr'] = file_err

            self._process = subprocess.Popen(self._cmd_str,
                                             shell=self._use_shell,
                                             cwd = subprocess_path,
                                             **kwargs)

            if self._signal is not signal.NSIG:
                try:
                    self._run_output, self._run_error = self._process.communicate(timeout=self._signal_sec)
                except subprocess.TimeoutExpired:
                    self._process.send_signal(self._signal)

            self._run_output, self._run_error = self._process.communicate()
            self._run_status = self._process.returncode

            if self._run_output is None:
                self._run_output = ""

            if self._run_error is None:
                self._run_error = ""

        except:
            self._run_error = traceback.format_exc()
            self._run_status = -1

        # Close any open files
        if file_out is not None:
            file_out.close()
        if file_err is not None:
            file_err.close()

####

    def _validate_cmd_str(self, cmd_str: str) -> None:
        """ Validate the cmd_str """
        if not isinstance(cmd_str, str):
            raise ValueError("ERROR: os_command() cmd_str must be a string")
        elif not cmd_str:
            raise ValueError("ERROR: os_command() cmd_str must not be empty")
        self._cmd_str = shlex.split(cmd_str)

####

    def _validate_output_path(self, file_path: Optional[str]) -> Optional[str]:
        """ Validate the output file path """
        if file_path is not None:
            dirpath = os.path.abspath(os.path.dirname(file_path))
            if not os.path.exists(dirpath):
                err_str = (("ERROR: os_command() Output path to file {0} ") +
                           ("is not valid")).format(file_path)
                raise ValueError(err_str)
        return file_path

################################################################################

class OSCommandResult:
    """ This class returns result data about the os_command that was executed """
    def __init__(self, cmd_str: List[str], status: int, output: str, error: str, timeout: bool) -> None:
        """
            Args:
                cmd_str (list[str]): The command to be executed
                status (int): The return status of the command execution.
                output (str): The standard output of the command execution.
                error (str): The error output of the command execution.
                timeout (bool): True if the command timed out during execution.
        """
        self._run_cmd_str = cmd_str
        self._run_status = status
        self._run_output = output
        self._run_error = error
        self._run_timeout = timeout

####

    def __repr__(self) -> str:
        rtn_str = (("Cmd = {0}; Status = {1}; Timeout = {2}; ") +
                   ("Error = {3}; Output = {4}")).format(self._run_cmd_str, \
                    self._run_status, self._run_timeout, self._run_error, \
                    self._run_output)
        return rtn_str

####

    def __str__(self) -> str:
        return self.__repr__()

####

    def cmd(self) -> List[str]:
        """ return the command that was run """
        return self._run_cmd_str

####

    def result(self) -> int:
        """ return the run status result """
        return self._run_status

####

    def output(self) -> str:
        """ return the run output result """
        # Sometimes the output can be a unicode or a byte string - convert it
        if isinstance(self._run_output, bytes):
            self._run_output = self._run_output.decode(encoding='UTF-8')
        return self._run_output

####

    def error(self) -> str:
        """ return the run error output result """
        # Sometimes the output can be a unicode or a byte string - convert it
        if isinstance(self._run_error, bytes):
            self._run_error = self._run_error.decode(encoding='UTF-8')
        return self._run_error

####

    def timeout(self) -> bool:
        """ return true if the run timed out """
        return self._run_timeout

# elements needs this while we're making this change
warn("OSCommand() is deprecated and will be removed in future versions of SST.\n" +
    "Use os_command() instead.",
    DeprecationWarning, stacklevel=2)
OSCommand = os_command

################################################################################

def os_simple_command(
    os_cmd: str,
    run_dir: Optional[str] = None,
    **kwargs: Any,
) -> Tuple[int, str]:
    """ Perform an simple os command and return a tuple of the (rtncode, rtnoutput).

        NOTE: Simple command cannot have pipes or redirects

        Args:
            os_cmd (str): Command to run
            run_dir (str): Directory where to run the command; if None (defaut)
                           current working directory is used.

        Returns:
            (tuple) Returns a tuple of the (rtncode, rtnoutput) of types (int, str)
    """
    warn("os_simple_command() is deprecated and will be removed in future versions of SST.\n" +
         "Use os_command() instead.",
         DeprecationWarning, stacklevel=2)
    check_param_type("os_cmd", os_cmd, str)
    if run_dir is not None:
        check_param_type("run_dir", run_dir, str)
    rtn = os_command(os_cmd, set_cwd=run_dir).run(**kwargs)
    rtn_data = (rtn.result(), rtn.output())
    return rtn_data

def os_ls(directory: str = ".", echo_out: bool = True, **kwargs: Any) -> str:
    """ Perform an simple ls -lia shell command and dump output to screen.

        Args:
            directory (str): Directory to run in [.]
            echo_out (bool): echo output to console [True]

        Returns:
            (str) Output from ls command
    """
    check_param_type("directory", directory, str)
    cmd = "ls -lia {0}".format(directory)
    rtn = os_command(cmd).run(**kwargs)
    if echo_out:
        log("{0}".format(rtn.output()))
    return rtn.output()

def os_pwd(echo_out: bool = True, **kwargs: Any) -> str:
    """ Perform an simple pwd shell command and dump output to screen.

        Args:
            echo_out (bool): echo output to console [True]

        Returns:
            (str) Output from pwd command
    """
    cmd = "pwd"
    rtn = os_command(cmd).run(**kwargs)
    if echo_out:
        log("{0}".format(rtn.output()))
    return rtn.output()

def os_cat(filepath: str, echo_out: bool = True, **kwargs: Any) -> str:
    """ Perform an simple cat shell command and dump output to screen.

        Args:
            filepath (str): Path to file to cat

        Returns:
            (str) Output from cat command
    """
    check_param_type("filepath", filepath, str)
    cmd = "cat {0}".format(filepath)
    rtn = os_command(cmd).run(**kwargs)
    if echo_out:
        log("{0}".format(rtn.output()))
    return rtn.output()

def os_symlink_file(srcdir: str, destdir: str, filename: str) -> None:
    """ Create a symlink of a file

        Args:
            srcdir (str): Path to source dir of the file
            destdir (str): Path to destination dir of the file
            filename (str): Name of the file
    """
    check_param_type("srcdir", srcdir, str)
    check_param_type("destdir", destdir, str)
    check_param_type("filename", filename, str)
    srcfilepath = "{0}/{1}".format(srcdir, filename)
    dstfilepath = "{0}/{1}".format(destdir, filename)
    os.symlink(srcfilepath, dstfilepath)

def os_symlink_dir(srcdir: str, destdir: str) -> None:
    """ Create a symlink of a directory

        Args:
            srcdir (str): Path to source dir
            destdir (str): Path to destination dir
    """
    check_param_type("srcdir", srcdir, str)
    check_param_type("destdir", destdir, str)
    os.symlink(srcdir, destdir)

def os_awk_print(in_str: str, fields_index_list: List[int]) -> str:
    """ Perform an awk / print (equivalent) command which returns specific
        fields of an input string as a string.

        Args:
            in_str (str): Input string to parse
            fields_index_list (list of ints): A list of ints and in the order of
                                             fields to be returned

        Returns:
            (str) Space separated string of extracted fields.
    """
    if isinstance(in_str, bytes):
        warn(
            "Passing bytes and not a string to os_awk_print() is deprecated and will be disallowed in future version of SST.",
            DeprecationWarning,
            stacklevel=2,
        )
        check_param_type("in_str", in_str, bytes)
    else:
        check_param_type("in_str", in_str, str)
    check_param_type("fields_index_list", fields_index_list, list)
    for index, field_index in enumerate(fields_index_list):
        check_param_type("field_index - {0}".format(index), field_index, int)
    finalstrdata = ""
    split_list = in_str.split()
    for field_index in fields_index_list:
        finalstrdata +=  "{0} ".format(split_list[field_index])
    return finalstrdata

def os_wc(in_file: str, fields_index_list: List[int] = [], **kwargs: Any) -> str:
    """ Run a wc (equivalent) command on a file and then extract specific
        fields of the result as a string.

        Args:
            in_file (str): Input string to parse
            fields_index_list (list of ints): A list of ints and in the order of
                                             fields to be returned

        Returns:
            (str) Space separated string of extracted fields.
    """
    check_param_type("in_file", in_file, str)
    check_param_type("fields_index_list", fields_index_list, list)
    for index, field_index in enumerate(fields_index_list):
        check_param_type("field_index - {0}".format(index), field_index, int)
    cmd = "wc {0}".format(in_file)
    rtn = os_command(cmd).run(**kwargs)
    wc_out = rtn.output()
    if fields_index_list:
        wc_out = os_awk_print(wc_out, fields_index_list)
    return wc_out

def os_test_file(file_path: str, expression: str = "-e", **kwargs: Any) -> bool:
    """ Run a shell 'test' command on a file.

        Args:
            file_path (str): Path to the file to be tested
            expression (str): Test expression [-e = exists]

        Returns:
            (bool) True if test is successful.
    """
    check_param_type("file_path", file_path, str)
    check_param_type("expression", expression, str)
    if os.path.exists(file_path):
        cmd = "test {0} {1}".format(expression, file_path)
        rtn = os_command(cmd).run(**kwargs)
        log_debug("Test cmd = {0}; rtn = {1}".format(cmd, rtn.result()))
        return rtn.result() == 0
    else:
        log_error("File {0} does not exist".format(file_path))
        return False

def os_wget(
    fileurl: str,
    targetdir: str,
    num_tries: int = 3,
    secsbetweentries: int = 10,
    wgetparams: str = "",
) -> bool:
    """ Perform a wget command to download a file from a url.

        Args:
            fileurl (str): URL to the file to be downloaded.
            targetdir (str): Where to download the file to
            num_tries (int): Max number of tries to download [3].
            secsbetweentries (int): Seconds between tries [10]
            wgetparams (str): Max number of tries to download.

        Returns:
            (bool) True if wget is successful.
    """
    check_param_type("fileurl", fileurl, str)
    check_param_type("targetdir", targetdir, str)
    check_param_type("num_tries", num_tries, int)
    check_param_type("secsbetweentries", secsbetweentries, int)
    check_param_type("wgetparams", wgetparams, str)

    wget_success = False

    wget_loc = which("wget")
    if wget_loc is None:
        log_error("wget is not installed")
        return False

    # Make sure target dir exists, and cd into it
    if not os.path.isdir(targetdir):
        log_error("Download directory {0} does not exist".format(targetdir))
        return False

    wgetoutfile = "{0}/wget.out".format(test_output_get_tmp_dir())

    log_debug("Downloading file via wget: {0}".format(fileurl))
    cmd = "{0} {1} --no-check-certificate -P {2} {3} > {4} 2>&1".\
       format(wget_loc, fileurl, targetdir, wgetparams, wgetoutfile)
    attemptnum = 1
    while attemptnum <= num_tries:
        log_debug("wget Attempt#{0}; cmd={1}".format(attemptnum, cmd))
        rtn_value = os.system(cmd)
        log_debug("wget rtn: {0}".format(rtn_value))
        if rtn_value == 0:
            # wget was successful, force exit of the loop
            log_debug("wget download successful")
            wget_success = True
            attemptnum = num_tries + 1
            continue

        if os.path.isfile(wgetoutfile):
            with open(wgetoutfile) as wgetfile:
                wgetoutput = "".join(wgetfile.readlines()[1:])

            log_debug("wget output:\n{0}".format(wgetoutput))
        else:
            log_debug("wget output: NOT FOUND")

        log_debug("Download Failed, waiting {0} seconds and trying again...".\
            format(secsbetweentries))

        attemptnum = attemptnum + 1
        time.sleep(secsbetweentries)

    if not wget_success:
        log_error("Failed to download via wget {0}".format(fileurl))

    return wget_success

def os_extract_tar(tarfilepath: str, targetdir: str = ".") -> bool:
    """ Extract directories/files from a tar file.

        Args:
            tarfilepath (str): The filepath to the tar file to be extracted.
            targetdir (str): Where to extract to [.]

        Returns:
            (bool) True if untar is successful.
    """
    if not os.path.isfile(tarfilepath):
        errmsg = "tar file{0} does not exist".format(tarfilepath)
        log_error(errmsg)
        return False
    try:
        this_tar = tarfile.open(tarfilepath)
        if sys.version_info.minor >= 12:
            this_tar.extractall(targetdir, filter="data")  # type: ignore [call-arg]
        else:
            this_tar.extractall(targetdir)
        this_tar.close()
        return True
    except tarfile.TarError as exc_e:
        errmsg = "Failed to untar file {0} - {1}".format(tarfilepath, exc_e)
        log_error(errmsg)
        return False

################################################################################
### Platform Specific Support Functions
################################################################################

def _get_linux_distribution() -> Tuple[str, str]:
    """Return the linux distribution info as a tuple"""
    # NOTE: order of checking is important
    distname = "undefined"
    distver = "undefined"
    if os.path.isfile("/etc/toss-release"):
        distname = "toss"
        distver = _get_linux_version("/etc/toss-release", "-")
    elif os.path.isfile("/etc/centos-release"):
        distname = "centos"
        distver = _get_linux_version("/etc/centos-release", " ")
    elif os.path.isfile("/etc/redhat-release"):
        distname = "red hat"
        distver = _get_linux_version("/etc/redhat-release", " ")
    elif os.path.isfile("/etc/rocky-release"):
        distname = "rocky"
        distver = _get_linux_version("/etc/rocky-release", " ")
    elif os.path.isfile("/etc/os-release"):
        distname, distver = _read_os_release("/etc/os-release")
    rtn_data = (distname, distver)
    return rtn_data

###

def _get_linux_version(filepath: str, sep: str) -> str:
    """ return the linux OS version as a string"""
    # Find the first digit + period in the tokenized string list
    with open(filepath, 'r') as filehandle:
        for line in filehandle:
            word_list = line.split(sep)
            for word in word_list:
                m_data = re.search(r"[\d.]+", word)
                if m_data is not None:
                    found_ver = m_data.string[m_data.start():m_data.end()]
                    return found_ver
    return "undefined"


def _read_os_release(filepath: str) -> Tuple[str, str]:
    """Read key-value pairs from a file that looks like /etc/os-release."""
    lines = Path(filepath).read_text(encoding="utf-8").splitlines()
    entries = dict()
    for line in lines:
        if line.strip() and not line.startswith("#"):
            key, value = line.strip().split("=", 1)
            entries[key] = value
    return entries["ID"], entries.get("VERSION_ID", "")


################################################################################
### Generic Internal Support Functions
################################################################################

def _get_sst_config_include_file_value(
    *,
    include_dict: Mapping[str, Union[str, int]],
    include_source: str,
    define: str,
    data_type: Type[T_include],
    default: Optional[T_include] = None,
    disable_warning: bool = False,
) -> T_include:
    """ Retrieve a define from an SST Configuration Include File (sst_config.h or sst-element_config.h)
       include_dict (dict): The dictionary to search for the define
       include_source (str): The name of the include file we are searching
       define (str): The define to look for
       default (str|int): Default Return if failure occurs
       data_type (str|int): The data type to return
       disable_warning (bool): Disable the warning if define not found
       :return (str|int): The returned data or default if not found in the dict
       This will raise a SSTTestCaseException if a default is not provided or type
       is incorrect
    """
    if data_type not in (int, str):
        raise SSTTestCaseException("Illegal datatype {0}".format(data_type))
    check_param_type("include_source", include_source, str)
    check_param_type("define", define, str)
    if default is not None:
        check_param_type("default", default, data_type)
    try:
        rtn_data = include_dict[define]
    except KeyError as exc_e:
        errmsg = f"Reading Config include file {include_source} - Cannot find #define {exc_e}"
        if not disable_warning:
            log_warning(errmsg)
        if default is None:
            raise SSTTestCaseException(exc_e)
        rtn_data = default
    return data_type(rtn_data)

###

def _get_sstsimulator_conf_value(
    *,
    section: str,
    key: str,
    data_type: Type[T_conf],
    default: Optional[T_conf] = None,
) -> T_conf:
    """ Retrieve a Section/Key from the SST Configuration File (sstsimulator.conf)
       section (str): The [section] to look for the key
       key (str): The key to find
       default (str|int|float|bool): Default Return if failure occurs
       :return (str|int|float|bool): The returned data or default if not found in file
       This will raise a SSTTestCaseException if a default is not provided
    """
    if data_type not in (int, str, float, bool):
        raise SSTTestCaseException("Illegal datatype {0}".format(data_type))
    check_param_type("section", section, str)
    check_param_type("key", key, str)
    if default is not None:
        check_param_type("default", default, data_type)
    core_conf_file_parser = test_engine_globals.TESTENGINE_CORE_CONFFILE_PARSER
    try:
        if data_type is str:
            return core_conf_file_parser.get(section, key)  # type: ignore [return-value]
        if data_type is int:
            return core_conf_file_parser.getint(section, key)  # type: ignore [return-value]
        if data_type is float:
            return core_conf_file_parser.getfloat(section, key)  # type: ignore [return-value]
        if data_type is bool:
            return core_conf_file_parser.getboolean(section, key)  # type: ignore [return-value]
    except configparser.Error as exc_e:
        rtn_default = _handle_config_err(exc_e, default)
        if default is None:
            raise SSTTestCaseException(exc_e)
    # to satisfy mypy
    assert rtn_default is not None
    return rtn_default

###

def _handle_config_err(
    exc_e: configparser.Error, default_rtn_data: Optional[T_conf]
) -> Optional[T_conf]:
    errmsg = "Reading SST-Core Config file sstsimulator.conf - {0}".format(exc_e)
    log_warning(errmsg)
    return default_rtn_data

###

def _remove_lines_with_string_from_file(removestring: str, input_filepath: str) -> None:
    bad_strings = [removestring]

    # Create a temp file
    filename = os.path.basename(input_filepath)
    tmp_output_filepath = "{0}/{1}.removed_lines".format(test_output_get_tmp_dir(), filename)
    bak_output_filepath = "{0}/{1}.bak".format(test_output_get_tmp_dir(), filename)

    if not os.path.exists(input_filepath):
        log_error("_remove_lines_with_string_from_file() - File {0} does not exist".\
            format(input_filepath))
        return

    # Copy the lines in the input file line by line, to a backup file and to the
    # tmp file; but skip any lines in the removestring from the tmp file
    with open(input_filepath) as oldfile, open(tmp_output_filepath, 'w') as newfile, open(bak_output_filepath, 'w') as bakfile:
        for line in oldfile:
            bakfile.write(line)
            if not any(bad_string in line for bad_string in bad_strings):
                newfile.write(line)

    # Now copy the tmp file back over the input file
    shutil.copy(tmp_output_filepath, input_filepath)
