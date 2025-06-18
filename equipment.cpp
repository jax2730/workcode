#include"bits/stdc++.h"
int main()
{
	std::map<std::string, std::string> charaterEquipment;
	std::cout<<"map created empty"<<(charaterEquipment.empty()?"yes":"NO")<<std::endl;
	std::cout<<"\n2. Equipping.."<<std::endl;
	charaterEquipment["Helmet"] = "Iron Helm";
	charaterEquipment["chest"] = "steel Armor";
	charaterEquipment["weapon"] = "Long Sword";
	
	std::cout<<"\n3. what weapon "<< std::endl;
	std::cout<<"weapon"<<charaterEquipment["weapon"]<<std::endl;
	
	std::cout<<"n4. a better sword!"<<std::endl;
	charaterEquipment["wepon"] = "Fire sword";
	
	std::cout<<"n5. current Equipment List:"<<std::endl;
	for(const auto& pair : charaterEquipment)
	{
		std::cout<<"-"<<pair.first<<":"<<pair.second<<std::endl;
	}
	std::cout<<"\6. Do we have gloves ?"<<std::endl;
	
	if(charaterEquipment.find("gloves") == charaterEquipment.end())
	{
		std::cout<<"NO gloves"<<std::endl;
	}else{
		std::cout<<"Gloves:" << charaterEquipment["Gloves"] << std::endl;
	}
	std::cout<< "\n-- Equipment List now --"<< std::endl;
	for(const auto& pair: charaterEquipment){
		//TODO
		std::cout<<"-"<<pair.first << ":" << pair.second << std::endl;
	}
	return 0;
	}
	
