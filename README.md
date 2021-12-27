# ToolApplication
# *Please clone/fork this repository to build your own ToolApplication in either ToolFramework or ToolDAQ* 

ToolApplication is a template repository for building your own applications in either: 

  ToolFramework:- an open source general modular C++ Framework.
  
  ToolDAQ:- DAQ based frameowrk built on ToolFramework


****************************
# Installation
****************************

Once clonned please run either:

*GetToolFramework.sh* to install dependances and files for creating a ToolFramework app

or

*GetToolDAQ.sh* to install dependances and files for creating a ToolDAQ app


****************************
# Concept
****************************

The main executable creates a ToolChain which is an object that holds Tools. Tools are added to the ToolChain and then the ToolChain can be told to Initialise Execute and Finalise each tool in the chain.

The ToolChain also holds a uesr defined DataModel which each tool has access too and can read ,update and modify. This is the method by which data is passed between Tools.

User Tools can be generated for use in the tool chain by incuding a Tool header.

For more information consult the ToolFramework manual:

https://drive.google.com/file/d/19F-nJpeq3cHJbjV4qiSk5qzpOa7p8keQ

or the docs

https://toolframework.github.io/ToolFrameworkApplication/
docs https://toolframework.github.io/ToolFrameworkCore/


Copyright (c) 2016 Benjamin Richards
