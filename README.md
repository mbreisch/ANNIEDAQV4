![toolapplication logo](https://user-images.githubusercontent.com/14093889/147496518-f3751cd6-0c57-4dd1-8517-3a02b61e59f5.png)

# *Please clone/fork this repository to build your own ToolApplication in either ToolFramework or ToolDAQ* 

ToolApplication is a template repository for building your own applications in either: 

  ToolFramework:- an open source general modular C++ Framework.
  
  or
  
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

The main executable creates a ToolChain which is an object that holds Tools. Tools are added to the ToolChain and then the ToolChain can be told to Initialise Execute and Finalise each Tool in the chain.

The ToolChain also holds a uesr defined DataModel which each Tool has access too and can read ,update and modify. This is the method by which data is passed between Tools.

User Tools can be generated for use in the ToolChain by included scripts.

For more information consult the ToolFramework manual:

https://drive.google.com/file/d/19F-nJpeq3cHJbjV4qiSk5qzpOa7p8keQ

or the Doxygen docs

https://toolframework.github.io/ToolApplication/

https://toolframework.github.io/ToolFrameworkCore/

http://tooldaq.github.io/ToolDAQFramework/

Copyright (c) 2016 Benjamin Richards (benjamin.richards@warwick.ac.uk)
