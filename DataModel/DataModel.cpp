#include "DataModel.h"

DataModel::DataModel(){
	postgres_helper.SetDataModel(this);
	//postgres_helper.SetVerbosity(10);
	//postgres.SetVerbosity(10);
}

/*
TTree* DataModel::GetTTree(std::string name){

  return m_trees[name];

}


void DataModel::AddTTree(std::string name,TTree *tree){

  m_trees[name]=tree;

}


void DataModel::DeleteTTree(std::string name,TTree *tree){

  m_trees.erase(name);

}

*/

