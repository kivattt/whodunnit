g++ -std=c++23 -O2 -c tests.cpp -o tests.o && g++ -std=c++23 -O2 tests.o -o tests -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lsfml-network && ./tests; rm tests tests.o
