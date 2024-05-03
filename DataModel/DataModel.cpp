#include "DataModel.h"

DataModel::DataModel(){
  	pgclient.SetDataModel(this);
  	postgres_helper.SetDataModel(this);
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

