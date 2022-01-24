![toolapplication logo](https://user-images.githubusercontent.com/14093889/147496518-f3751cd6-0c57-4dd1-8517-3a02b61e59f5.png)

# ***Please clone/fork this repository to build your own ToolApplication in either ToolFramework or ToolDAQ***

ToolApplication is a template repository for building your own applications in either: 

  - ToolFramework:- an open source general modular C++ Framework.
  - ToolDAQ:- DAQ focused frameowrk built on ToolFramework


****************************
# Installation
****************************

There are a few choices for installation, as mentioned this is an application template repository so the idea would be to fork or clone this repo into your own repo first, or build with it as a base. Some containers are provided for installation or trying out the code as well.

1. Using Docker / Singularity

   - For testing a complete install of Toolframework and ToolDAQ can be used by either:
     - ``` docker run --name=ToolFramework -it toolframework/toolframeworkapp```
     - ~~``` docker run --name=ToolDAQ -it toolframework/tooldaqapp```~~ in progress
   - If you want a container to use as a base for your own application container you can use either:
     - ```toolframework/centos7``` which is a light weight centos build with the prequisits to install ToolApplication
     - ```toolframework/core``` which is the same as above but with ToolFrameworkCore already installed in /opt/
     - ~~```tooldaq/core``` which is the same as above but with ToolFrameworkCore and ToolDAQframework already installed in /opt/~~ in progress

2. Install from source

   - Install Prequisits: 
     - RHEL/Centos... ``` yum install git make gcc-c++ zlib-devel dialog ```
     - Debian/Ubuntu.. ``` apt-get install git make g++ libz-dev dialog ```

   - Then clone the repo with ```git clone https://github.com/ToolFramework/ToolApplication.git``` or more likely your own fork

   - Once clonned please run either:

     - ```./GetToolFramework.sh``` to install dependances and files for creating a ToolFramework app
     - ```./GetToolDAQ.sh``` to install dependances and files for creating a ToolDAQ app


****************************
# Concept
****************************

The main executable creates a ToolChain which is an object that holds Tools. Tools are added to the ToolChain and then the ToolChain can be told to Initialise Execute and Finalise each Tool in the chain.

The ToolChain also holds a uesr defined DataModel which each Tool has access too and can read ,update and modify. This is the method by which data is passed between Tools.

User Tools can be generated for use in the ToolChain by included scripts.

For more information consult the ToolFramework manual:

https://drive.google.com/file/d/19F-nJpeq3cHJbjV4qiSk5qzpOa7p8keQ

or the Doxygen docs

- https://toolframework.github.io/ToolApplication/

- https://toolframework.github.io/ToolFrameworkCore/

- http://tooldaq.github.io/ToolDAQFramework/

Copyright (c) 2016 Benjamin Richards (benjamin.richards@warwick.ac.uk)
