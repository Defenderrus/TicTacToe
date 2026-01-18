#pragma once

#include <SFML/Graphics.hpp>

using namespace std;
using namespace sf;


class Button {
    private:
        RectangleShape shape;
        Text text;
        Color normalColor;
        Color hoverColor;
        bool wasPressed;

        void updateTextPosition();

    public:
        Button(const Vector2f &position, const Vector2f &size, const Font &font, const String &label, unsigned int charSize = 20);
        
        void update(const Vector2f &mousePos);
        void draw(RenderWindow &window) const;
        bool isClicked(const Vector2f &mousePos, bool mousePressed);
        void resetState();
        void setPosition(const Vector2f &position);
        void setLabel(const String &newLabel);
        void setSize(const Vector2f &newSize);
        FloatRect getGlobalBounds() const;
};
