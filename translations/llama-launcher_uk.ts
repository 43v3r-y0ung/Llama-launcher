<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="uk_UA">
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="14"/>
        <location filename="../mainwindow.cpp" line="158"/>
        <location filename="../mainwindow.cpp" line="346"/>
        <location filename="../mainwindow.cpp" line="354"/>
        <source>Llama Launcher</source>
        <translation>Llama Launcher</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="21"/>
        <source>Server</source>
        <translation>Сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="27"/>
        <source>llama-server:</source>
        <translation>llama-server:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="34"/>
        <source>llama-server (from PATH)</source>
        <translation>llama-server (з PATH)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="37"/>
        <source>Path to the llama-server executable. Leave empty to use the one found in PATH.</source>
        <translation>Шлях до виконуваного файлу llama-server. Залиште порожнім, щоб використати знайдений у PATH.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="44"/>
        <location filename="../mainwindow.ui" line="160"/>
        <source>Browse...</source>
        <translation>Огляд...</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="64"/>
        <source>Device:</source>
        <translation>Пристрій:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="71"/>
        <source>Device used for offloading (passed as --device). &quot;All GPUs&quot; lets llama-server split the model across every detected device, which is usually slower when one of them is an integrated GPU.</source>
        <translation>Пристрій для розвантаження моделі (передається як --device). «Усі GPU» дозволяє llama-server розподілити модель між усіма знайденими пристроями — зазвичай це повільніше, якщо один із них вбудована відеокарта.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="78"/>
        <source>Refresh</source>
        <translation>Оновити</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="85"/>
        <source>Port:</source>
        <translation>Порт:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="107"/>
        <source>Disable Web UI</source>
        <translation>Вимкнути вебінтерфейс</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="119"/>
        <source>Model</source>
        <translation>Модель</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="125"/>
        <source>Models folder:</source>
        <translation>Тека моделей:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="132"/>
        <source>Folder scanned for .gguf files</source>
        <translation>Тека, у якій шукати файли .gguf</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="139"/>
        <source>Choose...</source>
        <translation>Вибрати...</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="146"/>
        <source>Model:</source>
        <translation>Модель:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="153"/>
        <source>Models found in the models folder plus recently used ones. Settings below are remembered per model.</source>
        <translation>Моделі з теки моделей та нещодавно використані. Параметри нижче запам’ятовуються для кожної моделі окремо.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="196"/>
        <source>Parameters (remembered per model)</source>
        <translation>Параметри (окремо для кожної моделі)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="202"/>
        <source>Context Size:</source>
        <translation>Розмір контексту:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="248"/>
        <location filename="../mainwindow.ui" line="279"/>
        <location filename="../mainwindow.ui" line="304"/>
        <location filename="../mainwindow.ui" line="345"/>
        <source>Auto</source>
        <translation>Авто</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="251"/>
        <source>Use the native context size of the model (-c 0), reduced by --fit if memory is short.</source>
        <translation>Використати рідний розмір контексту моделі (-c 0); --fit зменшить його, якщо бракує пам’яті.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="260"/>
        <source>GPU Layers:</source>
        <translation>Шари на GPU:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="282"/>
        <source>Let llama-server offload as many layers as fit into VRAM (-ngl auto --fit on).</source>
        <translation>llama-server сам перенесе на GPU стільки шарів, скільки вміститься у відеопам’ять (-ngl auto --fit on).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="291"/>
        <source>MoE experts on CPU:</source>
        <translation>Експерти MoE на CPU:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="300"/>
        <source>For Mixture-of-Experts models: keep expert weights in system RAM so the rest of the model fits into VRAM. Auto leaves the decision to --fit.</source>
        <translation>Для моделей Mixture-of-Experts: тримати ваги експертів в оперативній пам’яті, щоб решта моделі вмістилася у відеопам’ять. «Авто» залишає рішення за --fit.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="309"/>
        <source>All experts (--cpu-moe)</source>
        <translation>Усі експерти (--cpu-moe)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="314"/>
        <source>First N layers (--n-cpu-moe)</source>
        <translation>Перші N шарів (--n-cpu-moe)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="337"/>
        <source>Flash Attention:</source>
        <translation>Flash Attention:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="350"/>
        <source>On</source>
        <translation>Увімк.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="355"/>
        <source>Off</source>
        <translation>Вимк.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="363"/>
        <source>Extra Arguments:</source>
        <translation>Додаткові аргументи:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="370"/>
        <source>e.g. --jinja --temp 0.7</source>
        <translation>напр. --jinja --temp 0.7</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="373"/>
        <source>Additional llama-server arguments, appended as-is. Quotes are supported.</source>
        <translation>Додаткові аргументи llama-server, додаються як є. Лапки підтримуються.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="380"/>
        <source>Context Shift</source>
        <translation>Зсув контексту (Context Shift)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="390"/>
        <source>Launcher</source>
        <translation>Запуск програми</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="396"/>
        <source>Launch at login (in tray)</source>
        <translation>Запускати при вході (у треї)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="403"/>
        <source>Start server when the launcher opens</source>
        <translation>Запускати сервер разом із програмою</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="410"/>
        <source>Notifications</source>
        <translation>Сповіщення</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="430"/>
        <location filename="../mainwindow.cpp" line="243"/>
        <source>Start Llama Server</source>
        <translation>Запустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="455"/>
        <location filename="../mainwindow.cpp" line="151"/>
        <source>Open in Browser</source>
        <translation>Відкрити в браузері</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="462"/>
        <source>Copy URL</source>
        <translation>Копіювати адресу</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="474"/>
        <source>Console output will appear here...</source>
        <translation>Тут з’явиться вивід сервера...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="42"/>
        <source>the program could not be started (file not found or not executable)</source>
        <translation>програму не вдалося запустити (файл не знайдено або він не виконуваний)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="44"/>
        <source>the process crashed</source>
        <translation>процес аварійно завершився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="46"/>
        <source>operation timed out</source>
        <translation>вичерпано час очікування</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="48"/>
        <source>write error</source>
        <translation>помилка запису</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="50"/>
        <source>read error</source>
        <translation>помилка читання</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="52"/>
        <source>unknown error</source>
        <translation>невідома помилка</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="100"/>
        <source>Server did not stop in time, killing it.</source>
        <translation>Сервер не зупинився вчасно — примусове завершення.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="149"/>
        <source>Start Server</source>
        <translation>Запустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="150"/>
        <source>Stop Server</source>
        <translation>Зупинити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="153"/>
        <source>Show/Restore</source>
        <translation>Показати/сховати</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="155"/>
        <source>Exit</source>
        <translation>Вийти</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="225"/>
        <source>Stopped</source>
        <translation>Зупинено</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="226"/>
        <source>Starting</source>
        <translation>Запуск</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="227"/>
        <source>Loading model</source>
        <translation>Завантаження моделі</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="228"/>
        <source>Running</source>
        <translation>Працює</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="243"/>
        <source>Stop Llama Server</source>
        <translation>Зупинити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="249"/>
        <source>Llama Launcher
Status: %1</source>
        <translation>Llama Launcher
Стан: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="251"/>
        <source>
Model: %1
%2</source>
        <translation>
Модель: %1
%2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="256"/>
        <source>Llama server is ready</source>
        <translation>Сервер готовий</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="256"/>
        <source>%1
%2</source>
        <translation>%1
%2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="269"/>
        <source>Copied %1</source>
        <translation>Скопійовано %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="344"/>
        <location filename="../mainwindow.cpp" line="352"/>
        <location filename="../mainwindow.cpp" line="833"/>
        <source>Warning</source>
        <translation>Попередження</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="344"/>
        <location filename="../mainwindow.cpp" line="346"/>
        <source>Please select a model first.</source>
        <translation>Спочатку виберіть модель.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="350"/>
        <source>Model file not found:
%1</source>
        <translation>Файл моделі не знайдено:
%1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="372"/>
        <source>Starting %1 %2</source>
        <translation>Запуск %1 %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="387"/>
        <source>Stopping server...</source>
        <translation>Зупинка сервера...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="419"/>
        <source>Process error: %1</source>
        <translation>Помилка процесу: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="423"/>
        <source>Could not start %1: %2</source>
        <translation>Не вдалося запустити %1: %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="427"/>
        <source>Llama server failed to start</source>
        <translation>Сервер не запустився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="441"/>
        <source>Server crashed.</source>
        <translation>Сервер аварійно завершився.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="443"/>
        <source>Server stopped (exit code %1).</source>
        <translation>Сервер зупинено (код виходу %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="448"/>
        <source>Server stopped unexpectedly (exit code %1).</source>
        <translation>Сервер несподівано зупинився (код виходу %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="451"/>
        <source>Llama server stopped</source>
        <translation>Сервер зупинився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="484"/>
        <source>Select llama-server</source>
        <translation>Виберіть llama-server</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="537"/>
        <source>Checking %1...</source>
        <translation>Перевірка %1...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="544"/>
        <source>%1 &amp;mdash; version %2</source>
        <translation>%1 &amp;mdash; версія %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="548"/>
        <source>Cannot run %1</source>
        <translation>Не вдається запустити %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="561"/>
        <source>All GPUs (auto)</source>
        <translation>Усі GPU (авто)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="575"/>
        <source>CPU only</source>
        <translation>Лише CPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="589"/>
        <source>%1 (not detected)</source>
        <translation>%1 (не знайдено)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="604"/>
        <source>Select Model</source>
        <translation>Виберіть модель</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="605"/>
        <source>GGUF Models (*.gguf);;All Files (*)</source>
        <translation>Моделі GGUF (*.gguf);;Усі файли (*)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="614"/>
        <source>Select Models Folder</source>
        <translation>Виберіть теку моделей</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="672"/>
        <source>(missing)</source>
        <translation>(відсутня)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="702"/>
        <source>No model selected.</source>
        <translation>Модель не вибрано.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="711"/>
        <source>Cannot read model metadata: %1</source>
        <translation>Не вдалося прочитати метадані моделі: %1</translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="720"/>
        <source>%n parts</source>
        <translation>
            <numerusform>%n частина</numerusform>
            <numerusform>%n частини</numerusform>
            <numerusform>%n частин</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="722"/>
        <source>%n layers</source>
        <translation>
            <numerusform>%n шар</numerusform>
            <numerusform>%n шари</numerusform>
            <numerusform>%n шарів</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="724"/>
        <source>native context %1</source>
        <translation>рідний контекст %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="726"/>
        <source>MoE: %1 experts, %2 active</source>
        <translation>MoE: %1 експертів, %2 активних</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="750"/>
        <source>all GPUs</source>
        <translation>усіх GPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="764"/>
        <source>Model (%1) fits into free VRAM on %2 (%3).</source>
        <translation>Модель (%1) вміщується у вільну відеопам’ять %2 (%3).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="768"/>
        <source>Model (%1) is larger than free VRAM on %2 (%3): part of it will run on the CPU, which is slower.</source>
        <translation>Модель (%1) більша за вільну відеопам’ять %2 (%3): частина виконуватиметься на CPU, а це повільніше.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="771"/>
        <source>For this MoE model, keeping the experts on the CPU (option below) usually gives the best speed.</source>
        <translation>Для цієї MoE-моделі найкращу швидкість зазвичай дає розміщення експертів на CPU (опція нижче).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="833"/>
        <source>Could not write %1</source>
        <translation>Не вдалося записати %1</translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../main.cpp" line="33"/>
        <source>Start hidden in the system tray (used at login).</source>
        <translation>Запуститися прихованим у треї (використовується при вході).</translation>
    </message>
</context>
</TS>
