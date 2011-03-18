# stage1_bamboo.sh

# Description:

# A quick and dirty shell script to command a build from the Atlassian
# Bamboo Continuous Integration Environment.

# Because there are pre-make steps that need to occur due to the use
# of the GNU Autotools, this script simplifies the build activation by
# consolidating the build steps.

# Assume that fresh code revision has been downloaded by Bamboo from
# the SST Google Code repository prior to invocation of this
# script. Plow through the build, exiting if something goes wrong.

./autogen.sh

lastexit=$?

if [ $lastexit -ne 0 ]
then
    exit $lastexit
fi

./configure --prefix=/usr/local --with-boost=/usr/local --with-zoltan=/usr/local --with-parmetis=/usr/local

lastexit=$?

if [ $lastexit -ne 0 ]
then
    exit $lastexit
fi

make all

lastexit=$?

if [ $lastexit -ne 0 ]
then
    exit $lastexit
fi


