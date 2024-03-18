# open62541_Unity
The project allows Unity3D/C# applications to implement full-featured support for the OPC UA architecture

OPC UA for Unity / C#

used in company projects https://Lcontent.ru

based on open62541

genaral OpenRTI repo: open62541 (https://github.com/open62541/open62541)

items:

DLLtoUnity - VisualStudio project DLL for Unity<->OPC communication

open62541 - open62541 library (clone https://github.com/open62541/open62541)

UnityTest - Unity test project (2021.3.8)

use:

Install cmake, install VisualStudio (recommended 2022)

Configuring an open62541 project with cmake

Building open62541

Assembly DLLtoUnity (do not forget to change the path to xml in the code)

Copy DLL files to plugins folder in Unity project

Run (server) on command line

Run Unity, open scene, test work

Состав:

DLLtoUnity - собственно DLL для связи Unity<->open62541

open62541 - библиотека OpenRTI (клон https://github.com/open62541/open62541)

UnityTest - тестовый проект Unity (2021.3.8)

Поддержка архитектуры OPC Unified Architecture — спецификации, определяющей передачу данных в промышленных сетях и взаимодействие устройств в них. Поскольку тренажеры очень часто имитируют место оператора (SCADA-системы), а там в свою очередь OPC UA является де-факто "стандартом".....

Порядок сборки:

Установка cmake, установка VisualStudio (рекомендуется 2022)

Конфигурация проекта open62541 при помощи cmake

Сборка open62541

Сборка DLLtoUnity (не забыть поменять путь до xml в коде)

Скопировать файлы DLL в папку plugins в проекте Unity

Запустить (сервер) в командной строке

Запустить Unity, открыть сцену, проверить работу

https://lcontent.ru/portfolio/otkrytaya-platforma/


