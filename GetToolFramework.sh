#!/bin/bash

init=1
toolframework=1
final=1

while [ ! $# -eq 0 ]
do
    case "$1" in
	--help | -h)
	    helpmenu
	    exit
	    ;;
	
	--no_init )
	     echo "Installing Applicaiton without creating Dependancy Folder"
	    init=0;
	    ;;

	--no_toolframework )
	    echo "Installing dependancies without ToolFramework"
	    toolframework=0
	    ;;

	--no_final )
            echo "Installing ToolFramework without compiling Applcation"
            final=0
            ;;

	
	--Final )
            echo "Compiling Apliciaion"
	    init=0
	    toolframework=0
            ;;

    esac
    shift
done

if [ $init -eq 1 ]
then
    
    mkdir Dependancies
fi

cd Dependancies

if [ $toolframework -eq 1 ]
then

git clone https://github.com/ToolFramework/ToolFrameworkCore.git
cd ToolFrameworkCore
make clean
make
export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
cd ../

fi

cd ../

if [ $final -eq 1 ]
then
    
    echo "current directory"
    echo `pwd`
    cp -r ./Dependancies/ToolFrameworkCore/DataModel/* ./DataModel
    cp -r ./Dependancies/ToolFrameworkCore/UserTools/* ./UserTools
    cp -r ./Dependancies/ToolFrameworkCore/configfiles/* ./configfiles
    cp ./Dependancies/ToolFrameworkCore/Makefile.Applcaition ./Makefile
    cp ./Dependancies/ToolFrameworkCore/CMakeLists.txt.Application ./CMakeLists.txt
    git add DataModel/*
    git add UserTools/*
    git add configfiles/*
    git add ./Makefile
    git add ./CMakeLists.txt
    make clean
    make 
    
    export LD_LIBRARY_PATH=`pwd`/lib:$LD_LIBRARY_PATH
fi
