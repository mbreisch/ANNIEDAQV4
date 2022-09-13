#include "Factory.h"

Tool* Factory(std::string tool){
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="DummyTool") ret=new DummyTool;

if (tool=="VME") ret=new VME;
if (tool=="PGTool") ret=new PGTool;
  if (tool=="StoreSave") ret=new StoreSave;
return ret;
}

