#include "typing_session.h"
#include "text_provider.h"
#include "console_handler.h"
#include "menu_handler.h"
#include <iostream>
#include <filesystem>

int main()
{
    try
    {
        ConsoleHandler console;
        MenuHandler menu(console);

        std::string selected_file = menu.showLanguageMenu();
        if (selected_file.empty())
        {
            return 0;
        }

        // Получаем язык из имени файла (без пути и расширения)
        std::filesystem::path filepath(selected_file);
        std::string language = filepath.stem().string();

        TextProvider textProvider(selected_file);
        TypingSession session(textProvider, console, language);

        session.start();

        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
}