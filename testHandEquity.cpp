

#include "handEquity.cpp"

int main()
{
	string hole_card = "9sTs";
	string community_card = "";
	int nb_players = 6;
	float win_rate = hand_equity(hole_card, community_card, nb_players, true);
	cout << "The estimated win rate is: " << win_rate << endl;
}
