<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="uk_UA">
<context>
    <name>ChartWidget</name>
    <message>
        <location filename="../chartwidget.cpp" line="299"/>
        <source>%1 s ago: </source>
        <translation>%1 с тому: </translation>
    </message>
    <message numerus="yes">
        <location filename="../chartwidget.cpp" line="346"/>
        <source>%n min</source>
        <translation>
            <numerusform>%n хв</numerusform>
            <numerusform>%n хв</numerusform>
            <numerusform>%n хв</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../chartwidget.cpp" line="346"/>
        <source>%n s</source>
        <translation>
            <numerusform>%n с</numerusform>
            <numerusform>%n с</numerusform>
            <numerusform>%n с</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="14"/>
        <location filename="../mainwindow.cpp" line="409"/>
        <location filename="../mainwindow.cpp" line="723"/>
        <location filename="../mainwindow.cpp" line="731"/>
        <source>Llama Launcher</source>
        <translation>Llama Launcher</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="286"/>
        <source>Server</source>
        <translation>Сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="294"/>
        <source>llama-server:</source>
        <translation>llama-server:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="301"/>
        <source>llama-server (from PATH)</source>
        <translation>llama-server (з PATH)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="304"/>
        <source>Path to the llama-server executable. Leave empty to use the one found in PATH.</source>
        <translation>Шлях до виконуваного файлу llama-server. Залиште порожнім, щоб використати знайдений у PATH.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="72"/>
        <location filename="../mainwindow.ui" line="311"/>
        <source>Browse...</source>
        <translation>Огляд...</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="65"/>
        <source>Models found in the models folder plus recently used ones. Parameters on the Model and Advanced tabs are remembered per model.</source>
        <translation>Моделі з теки моделей і нещодавно використані. Параметри на вкладках «Модель» і «Додатково» запам&apos;ятовуються для кожної моделі окремо.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="331"/>
        <source>Device:</source>
        <translation>Пристрій:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="338"/>
        <source>Device used for offloading (passed as --device). &quot;All GPUs&quot; lets llama-server split the model across every detected device, which is usually slower when one of them is an integrated GPU.</source>
        <translation>Пристрій для розвантаження моделі (передається як --device). «Усі GPU» дозволяє llama-server розподілити модель між усіма знайденими пристроями — зазвичай це повільніше, якщо один із них вбудована відеокарта.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="345"/>
        <source>Refresh</source>
        <translation>Оновити</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="352"/>
        <source>Port:</source>
        <translation>Порт:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="374"/>
        <source>Disable Web UI</source>
        <translation>Вимкнути вебінтерфейс</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="383"/>
        <source>Network:</source>
        <translation>Мережа:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="392"/>
        <source>Allow access from local network</source>
        <translation>Доступ із локальної мережі</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="395"/>
        <source>Listen on all network interfaces (--host 0.0.0.0) so other computers on your network can use the server.</source>
        <translation>Слухати на всіх мережевих інтерфейсах (--host 0.0.0.0), щоб інші комп’ютери у вашій мережі могли користуватися сервером.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="402"/>
        <source>API key:</source>
        <translation>Ключ API:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="412"/>
        <source>none</source>
        <translation>немає</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="415"/>
        <source>Clients must send this key (Authorization: Bearer &lt;key&gt;; the Web UI asks for it in its settings). Leave empty for no key.</source>
        <translation>Клієнти мають надсилати цей ключ (Authorization: Bearer &lt;ключ&gt;; вебінтерфейс запитує його у своїх налаштуваннях). Залиште порожнім, щоб працювати без ключа.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="424"/>
        <source>Generate</source>
        <translation>Згенерувати</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="25"/>
        <location filename="../mainwindow.ui" line="31"/>
        <source>Model</source>
        <translation>Модель</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="37"/>
        <source>Models folder:</source>
        <translation>Тека моделей:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="44"/>
        <source>Folder scanned for .gguf files</source>
        <translation>Тека, у якій шукати файли .gguf</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="51"/>
        <source>Choose...</source>
        <translation>Вибрати...</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="58"/>
        <source>Model:</source>
        <translation>Модель:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="108"/>
        <source>Parameters (remembered per model)</source>
        <translation>Параметри (окремо для кожної моделі)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="114"/>
        <source>Context Size:</source>
        <translation>Розмір контексту:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="160"/>
        <location filename="../mainwindow.ui" line="191"/>
        <location filename="../mainwindow.ui" line="216"/>
        <location filename="../mainwindow.ui" line="257"/>
        <location filename="../mainwindow.ui" line="583"/>
        <location filename="../mainwindow.ui" line="636"/>
        <location filename="../mainwindow.ui" line="669"/>
        <location filename="../mainwindow.ui" line="699"/>
        <source>Auto</source>
        <translation>Авто</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="163"/>
        <source>Use the native context size of the model (-c 0), reduced by --fit if memory is short.</source>
        <translation>Використати рідний розмір контексту моделі (-c 0); --fit зменшить його, якщо бракує пам’яті.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="172"/>
        <source>GPU Layers:</source>
        <translation>Шари на GPU:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="194"/>
        <source>Let llama-server offload as many layers as fit into VRAM (-ngl auto --fit on).</source>
        <translation>llama-server сам перенесе на GPU стільки шарів, скільки вміститься у відеопам’ять (-ngl auto --fit on).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="203"/>
        <source>MoE experts on CPU:</source>
        <translation>Експерти MoE на CPU:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="212"/>
        <source>For Mixture-of-Experts models: keep expert weights in system RAM so the rest of the model fits into VRAM. Auto leaves the decision to --fit.</source>
        <translation>Для моделей Mixture-of-Experts: тримати ваги експертів в оперативній пам’яті, щоб решта моделі вмістилася у відеопам’ять. «Авто» залишає рішення за --fit.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="221"/>
        <source>All experts (--cpu-moe)</source>
        <translation>Усі експерти (--cpu-moe)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="226"/>
        <source>First N layers (--n-cpu-moe)</source>
        <translation>Перші N шарів (--n-cpu-moe)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="249"/>
        <source>Flash Attention:</source>
        <translation>Flash Attention:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="262"/>
        <location filename="../mainwindow.ui" line="704"/>
        <source>On</source>
        <translation>Увімк.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="267"/>
        <location filename="../mainwindow.ui" line="709"/>
        <source>Off</source>
        <translation>Вимк.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="454"/>
        <source>Advanced</source>
        <translation>Додатково</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="969"/>
        <source>Extra Arguments:</source>
        <translation>Додаткові аргументи:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="979"/>
        <source>Additional llama-server arguments, appended as-is. Quotes are supported.</source>
        <translation>Додаткові аргументи llama-server, додаються як є. Лапки підтримуються.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="937"/>
        <source>Context Shift</source>
        <translation>Зсув контексту (Context Shift)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="485"/>
        <source>Memory and performance</source>
        <translation>Пам’ять і продуктивність</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="491"/>
        <source>KV cache type:</source>
        <translation>Тип KV-кешу:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="498"/>
        <source>Data type of the context (KV) cache, passed as -ctk/-ctv. q8_0 halves its memory use with almost no quality loss; q4_0 quarters it. A quantized cache needs Flash Attention.</source>
        <translation>Тип даних кешу контексту (KV), передається як -ctk/-ctv. q8_0 удвічі зменшує його розмір майже без втрати якості; q4_0 — учетверо. Квантований кеш потребує Flash Attention.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="502"/>
        <source>Default (f16)</source>
        <translation>Типовий (f16)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="507"/>
        <source>q8_0</source>
        <translation>q8_0</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="512"/>
        <source>q4_0</source>
        <translation>q4_0</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="530"/>
        <source>Keep KV cache in system RAM (--no-kv-offload)</source>
        <translation>Тримати KV-кеш в оперативній пам’яті (--no-kv-offload)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="533"/>
        <source>Saves VRAM for model layers at the cost of slower generation.</source>
        <translation>Звільняє відеопам’ять для шарів моделі ціною повільнішої генерації.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="540"/>
        <source>Lock model in RAM (--mlock)</source>
        <translation>Закріпити модель у пам’яті (--mlock)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="543"/>
        <source>Prevent the system from swapping the model out.</source>
        <translation>Не дозволяти системі вивантажувати модель у своп.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="550"/>
        <source>Load model fully instead of mapping it (--no-mmap)</source>
        <translation>Завантажувати модель повністю, без відображення файлу (--no-mmap)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="553"/>
        <source>Read the whole model into memory at start instead of memory-mapping the file.</source>
        <translation>Прочитати всю модель у пам’ять під час запуску замість відображення файлу в пам’ять.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="560"/>
        <source>CPU threads:</source>
        <translation>Потоки CPU:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="576"/>
        <source>Number of CPU threads used for generation (-t).</source>
        <translation>Кількість потоків CPU для генерації (-t).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="593"/>
        <source>Batch / micro-batch:</source>
        <translation>Батч / мікробатч:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="611"/>
        <source>Logical batch size (-b): how many prompt tokens are submitted at once.</source>
        <translation>Логічний розмір батча (-b): скільки токенів промпту подається за раз.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="627"/>
        <source>Physical batch size (-ub). Larger values speed up prompt processing but need more VRAM.</source>
        <translation>Фізичний розмір батча (-ub). Більші значення пришвидшують обробку промпту, але потребують більше відеопам’яті.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="646"/>
        <source>Parallel slots:</source>
        <translation>Паралельні слоти:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="662"/>
        <source>Number of requests served at the same time (-np). The context is split between the slots.</source>
        <translation>Скільки запитів обслуговується одночасно (-np). Контекст ділиться між слотами.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="682"/>
        <source>Chat</source>
        <translation>Чат</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="688"/>
        <source>Thinking:</source>
        <translation>Міркування:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="695"/>
        <source>Whether reasoning models think before answering (--reasoning). Auto follows the chat template.</source>
        <translation>Чи міркують моделі перед відповіддю (--reasoning). «Авто» — як визначає шаблон чату.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="717"/>
        <source>Thinking budget:</source>
        <translation>Бюджет міркувань:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="733"/>
        <source>Maximum number of thinking tokens (--reasoning-budget).</source>
        <translation>Максимальна кількість токенів на міркування (--reasoning-budget).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="740"/>
        <source>Unlimited</source>
        <translation>Без обмежень</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="750"/>
        <source>Jinja chat template</source>
        <translation>Шаблон чату Jinja</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="753"/>
        <source>Use the chat template embedded in the model (needed for tool calling). Unchecked passes --no-jinja.</source>
        <translation>Використовувати шаблон чату, вбудований у модель (потрібен для виклику інструментів). Якщо вимкнено, передається --no-jinja.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="763"/>
        <source>Vision:</source>
        <translation>Зір:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="770"/>
        <source>Load multimodal projector</source>
        <translation>Завантажити мультимодальний проєктор</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="773"/>
        <source>Load the mmproj*.gguf file found next to the model, so the model can see images (--mmproj).</source>
        <translation>Завантажити файл mmproj*.gguf, що лежить поруч із моделлю, щоб модель могла бачити зображення (--mmproj).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="787"/>
        <source>Keep projector on CPU (--no-mmproj-offload)</source>
        <translation>Тримати проєктор на CPU (--no-mmproj-offload)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="790"/>
        <source>Saves VRAM; image processing becomes slower.</source>
        <translation>Економить відеопам’ять; обробка зображень стає повільнішою.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="800"/>
        <source>Default sampling</source>
        <translation>Семплінг за замовчуванням</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="806"/>
        <source>Checked values replace the server defaults; clients can still override them per request.</source>
        <translation>Позначені значення замінюють типові значення сервера; клієнти все одно можуть змінити їх у кожному запиті.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="816"/>
        <source>Temperature</source>
        <translation>Температура</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="849"/>
        <source>Top P</source>
        <translation>Top P</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="875"/>
        <source>Top K</source>
        <translation>Top K</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="902"/>
        <source>Min P</source>
        <translation>Min P</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="931"/>
        <source>Other</source>
        <translation>Інше</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="944"/>
        <source>Prometheus metrics endpoint (--metrics)</source>
        <translation>Метрики Prometheus (--metrics)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="947"/>
        <source>Expose /metrics with token counters and throughput.</source>
        <translation>Відкрити /metrics з лічильниками токенів і швидкістю.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="954"/>
        <source>Skip warm-up run (--no-warmup)</source>
        <translation>Без прогрівального запуску (--no-warmup)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="957"/>
        <source>Start faster by skipping the empty warm-up pass.</source>
        <translation>Швидший старт без порожнього прогрівального проходу.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="976"/>
        <source>e.g. --seed 42 --repeat-penalty 1.1</source>
        <translation>напр. --seed 42 --repeat-penalty 1.1</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1000"/>
        <source>Monitor</source>
        <translation>Монітор</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1036"/>
        <source>Log</source>
        <translation>Журнал</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1061"/>
        <source>Copy Log</source>
        <translation>Копіювати журнал</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1068"/>
        <source>Clear</source>
        <translation>Очистити</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1078"/>
        <source>Launcher</source>
        <translation>Запуск програми</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1084"/>
        <source>Launch at login (in tray)</source>
        <translation>Запускати при вході (у треї)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1091"/>
        <source>Start server when the launcher opens</source>
        <translation>Запускати сервер разом із програмою</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1098"/>
        <source>Notifications</source>
        <translation>Сповіщення</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1121"/>
        <location filename="../mainwindow.cpp" line="533"/>
        <source>Start Llama Server</source>
        <translation>Запустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1160"/>
        <location filename="../mainwindow.cpp" line="402"/>
        <source>Open in Browser</source>
        <translation>Відкрити в браузері</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1167"/>
        <source>Copy URL</source>
        <translation>Копіювати адресу</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1045"/>
        <source>Console output will appear here...</source>
        <translation>Тут з’явиться вивід сервера...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="90"/>
        <source>the program could not be started (file not found or not executable)</source>
        <translation>програму не вдалося запустити (файл не знайдено або він не виконуваний)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="92"/>
        <source>the process crashed</source>
        <translation>процес аварійно завершився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="94"/>
        <source>operation timed out</source>
        <translation>вичерпано час очікування</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="96"/>
        <source>write error</source>
        <translation>помилка запису</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="98"/>
        <source>read error</source>
        <translation>помилка читання</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="100"/>
        <source>unknown error</source>
        <translation>невідома помилка</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="166"/>
        <source>Server did not stop in time, killing it.</source>
        <translation>Сервер не зупинився вчасно — примусове завершення.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="400"/>
        <source>Start Server</source>
        <translation>Запустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="401"/>
        <source>Stop Server</source>
        <translation>Зупинити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="404"/>
        <source>Show/Restore</source>
        <translation>Показати/сховати</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="406"/>
        <source>Exit</source>
        <translation>Вийти</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="503"/>
        <source>Stopped</source>
        <translation>Зупинено</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="504"/>
        <source>Starting</source>
        <translation>Запуск</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="379"/>
        <location filename="../mainwindow.cpp" line="505"/>
        <source>Loading model</source>
        <translation>Завантаження моделі</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="257"/>
        <source>%1 GiB</source>
        <translation>%1 ГіБ</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="258"/>
        <location filename="../mainwindow.cpp" line="391"/>
        <source>%1 tok/s</source>
        <translation>%1 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="260"/>
        <location filename="../mainwindow.cpp" line="283"/>
        <source>CPU</source>
        <translation>CPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="261"/>
        <location filename="../mainwindow.cpp" line="266"/>
        <source>System</source>
        <translation>Система</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="265"/>
        <source>RAM</source>
        <translation>ОЗП</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="269"/>
        <source>GPU load</source>
        <translation>Завантаження GPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="273"/>
        <source>VRAM</source>
        <translation>Відеопам’ять</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="276"/>
        <source>Generation speed</source>
        <translation>Швидкість генерації</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="285"/>
        <source>GPU</source>
        <translation>GPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="290"/>
        <source>Last minute; details on the Monitor tab</source>
        <translation>Остання хвилина; докладніше на вкладці «Монітор»</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="293"/>
        <location filename="../mainwindow.cpp" line="368"/>
        <source>The server is not running.</source>
        <translation>Сервер не запущено.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="334"/>
        <source>%1, last minute; details on the Monitor tab</source>
        <translation>%1, остання хвилина; докладніше на вкладці «Монітор»</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="339"/>
        <source>sleeping (power saving)</source>
        <translation>сплячий режим (енергозбереження)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="344"/>
        <source>%1 W</source>
        <translation>%1 Вт</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="346"/>
        <source>%1 MHz</source>
        <translation>%1 МГц</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="353"/>
        <source>Shared system memory (GTT): %1 GiB</source>
        <translation>Спільна системна пам’ять (GTT): %1 ГіБ</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="362"/>
        <source>generation %1 tok/s</source>
        <translation>генерація %1 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="364"/>
        <source>prompt %1 tok/s</source>
        <translation>промпт %1 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="365"/>
        <source>Last request: %1</source>
        <translation>Останній запит: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="374"/>
        <source>Reading prompt: &lt;b&gt;%1 %&lt;/b&gt; at %2 tok/s</source>
        <translation>Обробка промпту: &lt;b&gt;%1 %&lt;/b&gt;, %2 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="377"/>
        <source>Generating: &lt;b&gt;%1 tok/s&lt;/b&gt;</source>
        <translation>Генерація: &lt;b&gt;%1 ток/с&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="379"/>
        <source>Idle</source>
        <translation>Очікує запитів</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="381"/>
        <source>Context: %1 / %2 (%3 %)</source>
        <translation>Контекст: %1 / %2 (%3 %)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="384"/>
        <source>llama-server uses %1 GiB RAM</source>
        <translation>llama-server займає %1 ГіБ ОЗП</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="506"/>
        <source>Running</source>
        <translation>Працює</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="518"/>
        <source>network:</source>
        <translation>мережа:</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="533"/>
        <source>Stop Llama Server</source>
        <translation>Зупинити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="539"/>
        <source>Llama Launcher
Status: %1</source>
        <translation>Llama Launcher
Стан: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="541"/>
        <source>
Model: %1
%2</source>
        <translation>
Модель: %1
%2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="549"/>
        <source>Llama server is ready</source>
        <translation>Сервер готовий</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="550"/>
        <source>%1
%2</source>
        <translation>%1
%2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="567"/>
        <source>Copied %1</source>
        <translation>Скопійовано %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="588"/>
        <source>no network connection found</source>
        <translation>мережеве з’єднання не знайдено</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="590"/>
        <source>Other computers can connect at %1</source>
        <translation>Інші комп’ютери можуть підключатися за адресою %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="593"/>
        <source>No API key: anyone on the network can use the server. Set a key, especially on public Wi-Fi.</source>
        <translation>Ключа API немає: будь-хто в мережі зможе користуватися сервером. Задайте ключ, особливо в публічних Wi-Fi.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="721"/>
        <location filename="../mainwindow.cpp" line="729"/>
        <location filename="../mainwindow.cpp" line="1270"/>
        <source>Warning</source>
        <translation>Попередження</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="721"/>
        <location filename="../mainwindow.cpp" line="723"/>
        <source>Please select a model first.</source>
        <translation>Спочатку виберіть модель.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="727"/>
        <source>Model file not found:
%1</source>
        <translation>Файл моделі не знайдено:
%1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="757"/>
        <source>Starting %1 %2</source>
        <translation>Запуск %1 %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="774"/>
        <source>Stopping server...</source>
        <translation>Зупинка сервера...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="805"/>
        <source>Log copied to clipboard</source>
        <translation>Журнал скопійовано в буфер обміну</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="818"/>
        <source>Process error: %1</source>
        <translation>Помилка процесу: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="822"/>
        <source>Could not start %1: %2</source>
        <translation>Не вдалося запустити %1: %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="827"/>
        <source>Llama server failed to start</source>
        <translation>Сервер не запустився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="841"/>
        <source>Server crashed.</source>
        <translation>Сервер аварійно завершився.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="843"/>
        <source>Server stopped (exit code %1).</source>
        <translation>Сервер зупинено (код виходу %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="849"/>
        <source>Server stopped unexpectedly (exit code %1).</source>
        <translation>Сервер несподівано зупинився (код виходу %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="852"/>
        <source>Llama server stopped</source>
        <translation>Сервер зупинився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="885"/>
        <source>Select llama-server</source>
        <translation>Виберіть llama-server</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="938"/>
        <source>Checking %1...</source>
        <translation>Перевірка %1...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="945"/>
        <source>%1 &amp;mdash; version %2</source>
        <translation>%1 &amp;mdash; версія %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="949"/>
        <source>Cannot run %1</source>
        <translation>Не вдається запустити %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="962"/>
        <source>All GPUs (auto)</source>
        <translation>Усі GPU (авто)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="978"/>
        <source>CPU only</source>
        <translation>Лише CPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="992"/>
        <source>%1 (not detected)</source>
        <translation>%1 (не знайдено)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1007"/>
        <source>Select Model</source>
        <translation>Виберіть модель</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1008"/>
        <source>GGUF Models (*.gguf);;All Files (*)</source>
        <translation>Моделі GGUF (*.gguf);;Усі файли (*)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1017"/>
        <source>Select Models Folder</source>
        <translation>Виберіть теку моделей</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1075"/>
        <source>(missing)</source>
        <translation>(відсутня)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1105"/>
        <source>No mmproj*.gguf file next to the model.</source>
        <translation>Поруч із моделлю немає файлу mmproj*.gguf.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1109"/>
        <source>No model selected.</source>
        <translation>Модель не вибрано.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1118"/>
        <source>Cannot read model metadata: %1</source>
        <translation>Не вдалося прочитати метадані моделі: %1</translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="1127"/>
        <source>%n parts</source>
        <translation>
            <numerusform>%n частина</numerusform>
            <numerusform>%n частини</numerusform>
            <numerusform>%n частин</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="1129"/>
        <source>%n layers</source>
        <translation>
            <numerusform>%n шар</numerusform>
            <numerusform>%n шари</numerusform>
            <numerusform>%n шарів</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1131"/>
        <source>native context %1</source>
        <translation>рідний контекст %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1133"/>
        <source>MoE: %1 experts, %2 active</source>
        <translation>MoE: %1 експертів, %2 активних</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1157"/>
        <source>all GPUs</source>
        <translation>усіх GPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1171"/>
        <source>Model (%1) fits into free VRAM on %2 (%3).</source>
        <translation>Модель (%1) вміщується у вільну відеопам’ять %2 (%3).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1175"/>
        <source>Model (%1) is larger than free VRAM on %2 (%3): part of it will run on the CPU, which is slower.</source>
        <translation>Модель (%1) більша за вільну відеопам’ять %2 (%3): частина виконуватиметься на CPU, а це повільніше.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1178"/>
        <source>For this MoE model, keeping the experts on the CPU (option below) usually gives the best speed.</source>
        <translation>Для цієї MoE-моделі найкращу швидкість зазвичай дає розміщення експертів на CPU (опція нижче).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1215"/>
        <source>A quantized KV cache needs Flash Attention: set it to Auto or On on the Model tab.</source>
        <translation>Квантований KV-кеш потребує Flash Attention: встановіть «Авто» або «Увімк.» на вкладці «Модель».</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1270"/>
        <source>Could not write %1</source>
        <translation>Не вдалося записати %1</translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="1479"/>
        <source>Moved %n option(s) from Extra Arguments to the Advanced tab</source>
        <translation>
            <numerusform>Перенесено %n опцію з додаткових аргументів на вкладку «Додатково»</numerusform>
            <numerusform>Перенесено %n опції з додаткових аргументів на вкладку «Додатково»</numerusform>
            <numerusform>Перенесено %n опцій з додаткових аргументів на вкладку «Додатково»</numerusform>
        </translation>
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
