#!/bin/bash

# Проверка и установка необходимых зависимостей
check_and_install() {
    if ! dpkg -l | grep -q $1; then
        echo "Установка $1..."
        sudo apt-get install -y $1
    fi
}

# Проверка наличия sudo прав
if [ "$EUID" -ne 0 ]; then 
    echo "Для установки зависимостей потребуются права sudo"
fi

# Обновление списка пакетов
sudo apt-get update

# Установка необходимых пакетов
check_and_install "build-essential"
check_and_install "libncurses5-dev"

# Сборка проекта
make clean
make

echo "Установка завершена. Программа собрана в build/typing_trainer" 