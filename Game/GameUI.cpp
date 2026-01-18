#include "GameUI.hpp"


void Button::updateTextPosition() {
    FloatRect textBounds = text.getLocalBounds();
    Vector2f pos = shape.getPosition();
    Vector2f size = shape.getSize();
    text.setPosition(Vector2f(pos.x + (size.x - textBounds.size.x) / 2, pos.y + (size.y - textBounds.size.y) / 2 - 5));
}

Button::Button(const Vector2f &position, const Vector2f &size, const Font &font, const String &label, unsigned int charSize):
    wasPressed(false), text(font, label, charSize) {
    
    shape.setSize(size);
    shape.setPosition(position);
    shape.setFillColor(Color(70, 70, 70));
    shape.setOutlineThickness(2);
    shape.setOutlineColor(Color(100, 100, 100));

    text.setFillColor(Color::White);
    updateTextPosition();
    
    normalColor = Color(70, 70, 70);
    hoverColor = Color(100, 100, 100);
}

void Button::update(const Vector2f &mousePos) {
    bool isHovered = shape.getGlobalBounds().contains(mousePos);
    shape.setFillColor(isHovered ? hoverColor : normalColor);
    shape.setOutlineColor(isHovered ? Color(150, 150, 150) : Color(100, 100, 100));
}

void Button::draw(RenderWindow &window) const {
    window.draw(shape);
    window.draw(text);
}

bool Button::isClicked(const Vector2f &mousePos, bool mousePressed) {
    bool currentlyHovered = shape.getGlobalBounds().contains(mousePos);
    
    if (currentlyHovered && mousePressed && !wasPressed) {
        wasPressed = true;
        return true;
    }
    
    if (!mousePressed) {
        wasPressed = false;
    }
    
    return false;
}

void Button::resetState() {
    wasPressed = false;
    shape.setFillColor(normalColor);
    shape.setOutlineColor(Color(100, 100, 100));
}

void Button::setPosition(const Vector2f &position) {
    shape.setPosition(position);
    updateTextPosition();
}

void Button::setLabel(const String &newLabel) {
    text.setString(newLabel);
    updateTextPosition();
}

void Button::setSize(const Vector2f &newSize) {
    shape.setSize(newSize);
    updateTextPosition();
}

FloatRect Button::getGlobalBounds() const {
    return shape.getGlobalBounds();
}
