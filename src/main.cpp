#include "typing_session.h"
#include "text_provider.h"
#include "console_handler.h"
#include <iostream>

int main() {
    try {
        ConsoleHandler console;
        TextProvider textProvider("data/texts.txt");
        TypingSession session(textProvider, console);
        
        session.start();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
} 