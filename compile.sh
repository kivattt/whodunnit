#g++ -std=c++23 -O2 -c main.cpp -o whodunnit.o && g++ -std=c++23 -O2 whodunnit.o -o whodunnit -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network

# -g debug flag
g++ -std=c++23 -O2 -g -c main.cpp -o whodunnit.o && g++ -std=c++23 -O2 -g whodunnit.o -o whodunnit -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network
