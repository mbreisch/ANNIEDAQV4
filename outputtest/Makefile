all:
	 g++ main.cpp -I ../include/ -L ../lib/ -lStore -lDataModel -lServiceDiscovery -L ../Dependencies/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams -lboost_system -I ../Dependencies/boost_1_66_0/install/include -L ../Dependencies/zeromq-4.0.7/lib -lzmq -I ../Dependencies/zeromq-4.0.7/include

marcus: main_marcus.cpp
	g++ -std=c++11 -g main_marcus.cpp -I ../include -L ../lib -lStore -lDataModel -lServiceDiscovery -L ../Dependencies/boost_1_66_0/install/lib -lboost_date_time -lboost_serialization -lboost_iostreams -lboost_system -I ../Dependencies/boost_1_66_0/install/include -I ../Dependencies/zeromq-4.0.7/include -o $@ #`root-config --cflags --libs`
