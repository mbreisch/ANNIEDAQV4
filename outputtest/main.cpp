#include <iostream>
#include <BoostStore.h>
#include <MRDOut.h>
#include <CardData.h>

int main(int argc, char* argv[]){


  BoostStore bs(false, 0);
  std::string file=argv[1];
  bs.Initialise(file);


  std::cout<<"in RAW file:"<<std::endl;
  bs.Print(false);

  std::cout<<std::endl<<std::endl;

  BoostStore* CCData= new BoostStore(false, 2);;

  bs.Get("CAMAC", *CCData);

  std::cout<<"in CCData:"<<std::endl;
  CCData->Print(false);

  std::cout<<std::endl<<std::endl;

  std::cout<<"in CCData Header:"<<std::endl;
  CCData->Header->Print(false);

  std::cout<<std::endl<<std::endl;

  unsigned long entries=0;
  CCData->Header->Get("TotalEntries", entries);

  std::cout<<"entries="<<entries<<std::endl<<std::endl;

  for(int evt=0; evt<4; evt++){
    //    CCData->Delete();    
    CCData->GetEntry(evt);

    MRDOut MRDout;
    
    CCData->Get("Data",MRDout);
    
    std::cout<<"size="<<MRDout.Crate.size()<<std::endl;
        
    for(int i=0; i<MRDout.Crate.size(); i++){
      
      std::cout<<"i="<<i<<", Crate="<<MRDout.Crate.at(i)<<", Slot="<<MRDout.Slot.at(i)<<", Channel="<<MRDout.Channel.at(i)<<std::endl;
    }
    
    
  }
  ////////////////////////////////////////////////////////////
  
  std::cout<<std::endl<<std::endl;
  BoostStore* VME=new  BoostStore(false, 2);
  
  
  bs.Get("PMTData", *VME);
  std::cout<<"in VME:"<<std::endl;
  VME->Print(false);
  
  std::cout<<std::endl<<std::endl;

  std::cout<<"in VME Header:"<<std::endl;
  VME->Header->Print(false);

  std::cout<<std::endl<<std::endl;

  unsigned long entries2=0;
  VME->Header->Get("TotalEntries", entries2);

  std::cout<<"entries="<<entries2<<std::endl<<std::endl;

  for(int evt=0; evt<4; evt++){
    VME->GetEntry(evt);
    
    std::vector<CardData> tmp;
    
    VME->Get("CardData", tmp);
    
    std::cout<<"carddata size="<<tmp.size()<<std::endl;

    for(int i=0;i<tmp.size(); i++){
      std::cout<<"d1"<<std::endl;
      std::cout<<"CardID="<<tmp.at(i).CardID<<std::endl;
      std::cout<<"Data.size="<<tmp.at(i).Data.size()<<std::endl;      
      std::cout<<"Data.at(0)="<<tmp.at(i).Data.at(0)<<std::endl;
    }
  }

  return 0;
  
}
