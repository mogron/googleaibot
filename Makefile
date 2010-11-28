all:
	g++ -O3 -funroll-loops -o CppStarter abstractbot.cpp comparator.cpp filter.cpp fleet.cpp game.cpp main.cpp MyBot.cc order.cpp parser.cpp planet.cpp player.cpp point2d.cpp stringtokenizer.cpp stlastar.h knapsackTarget.h 
