#include"iostream"
class Player {
	int m_helth = 100;
public:
	void Damage(int amount) {
		m_helth -= amount;
		if(m_helth < 0) m_helth = 0;
	}
	
	int GetHealth() const
	{
		return m_helth;
	}
	
	void YourMethodOrFunction() {
		
	}
};

int main()
{
	Player Player;
	std::cout<<Player.GetHealth()<<std::endl;
	Player.Damage(10);
	std::cout<<"10"<<std::endl;
	
	
	int curentHealth = Player.GetHealth();
	std::cout<<curentHealth<<std::endl;
	
	return 0;
}
