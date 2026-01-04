# Компилятор
CXX = g++
CXXFLAGS = -std=c++17 -pthread -mwindows -I. -I./SFML-3.0.2/include

# Пути к библиотекам SFML
SFML_LIBS = -L./SFML-3.0.2/lib -lsfml-graphics -lsfml-window -lsfml-system

# Все исходные файлы
SRCS = main.cpp

# Цели
all:
	main
	start

main:
	$(CXX) $(CXXFLAGS) $(SRCS) $(SFML_LIBS) -o main.exe

start:
	./main.exe

clean:
	rm -f main.exe

.PHONY: all compile run clean