#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="CrateReaderStream3") ret=new CrateReaderStream3;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="RunControl") ret=new RunControl;
if (tool=="VMESendDataStream2") ret=new VMESendDataStream2;
return ret;
}
