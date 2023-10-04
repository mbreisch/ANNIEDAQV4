#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="ACC_DataRead") ret=new ACC_DataRead;
if (tool=="ACC_Receive") ret=new ACC_Receive;
if (tool=="ACC_SetupBoards") ret=new ACC_SetupBoards;
if (tool=="ACC_Stream") ret=new ACC_Stream;
if (tool=="SeeConnectedBoards") ret=new SeeConnectedBoards;
if (tool=="RunControl") ret=new RunControl;
if (tool=="test") ret=new test;
if (tool=="PGStarter") ret=new PGStarter;
return ret;
}
