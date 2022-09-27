#include "Factory.h"

Tool* Factory(std::string tool) {
Tool* ret=0;

// if (tool=="Type") tool=new Type;
if (tool=="CAMAC") ret=new CAMAC;
if (tool=="DummyTool") ret=new DummyTool;
if (tool=="LAPPD") ret=new LAPPD;
if (tool=="PGStarter") ret=new PGStarter;
if (tool=="RunControl") ret=new RunControl;
if (tool=="SlackBot") ret=new SlackBot;
if (tool=="StoreSave") ret=new StoreSave;
if (tool=="VME") ret=new VME;
return ret;
}
