#!/bin/bash

#Application path location of applicaiton

Dependancies=`pwd`/Dependancies

export LD_LIBRARY_PATH=`pwd`/lib:${Dependancies}/ToolFramework/lib:$LD_LIBRARY_PATH

export SEGFAULT_SIGNALS="all"
