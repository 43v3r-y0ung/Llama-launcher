<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="uk_UA">
<context>
    <name>ChartWidget</name>
    <message>
        <location filename="../chartwidget.cpp" line="271"/>
        <source>%1 s ago: </source>
        <translation>%1 с тому: </translation>
    </message>
    <message numerus="yes">
        <location filename="../chartwidget.cpp" line="318"/>
        <source>%n min</source>
        <translation>
            <numerusform>%n хв</numerusform>
            <numerusform>%n хв</numerusform>
            <numerusform>%n хв</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../chartwidget.cpp" line="318"/>
        <source>%n s</source>
        <translation>
            <numerusform>%n с</numerusform>
            <numerusform>%n с</numerusform>
            <numerusform>%n с</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>LogAdvisor</name>
    <message>
        <location filename="../logadvisor.cpp" line="30"/>
        <source>Automatic fitting was skipped because the number of GPU layers is set manually, so the context size was not reduced to fit into memory. Set GPU layers to Auto, or choose the context size yourself.</source>
        <translation>Автоматичне підлаштування пропущено, бо кількість шарів на GPU задано вручну, тому розмір контексту не зменшено під доступну пам&apos;ять. Увімкніть «Авто» для шарів GPU або задайте розмір контексту самостійно.</translation>
    </message>
    <message>
        <location filename="../logadvisor.cpp" line="39"/>
        <source>The server turned Flash Attention off by itself, as this device does not support it for this configuration. Leave Flash Attention on Auto: there is no need to turn it off by hand, and a quantized KV cache does not work with Flash Attention forced off.</source>
        <translation>Сервер сам вимкнув Flash Attention, бо цей пристрій не підтримує його для такої конфігурації. Лишіть для Flash Attention «Авто»: вимикати його вручну не потрібно, а квантований KV-кеш не працює, якщо Flash Attention примусово вимкнено.</translation>
    </message>
    <message>
        <location filename="../logadvisor.cpp" line="47"/>
        <source>This model does not support Context Shift, so that option has no effect and can be turned off.</source>
        <translation>Ця модель не підтримує Context Shift, тож ця опція не діє і її можна вимкнути.</translation>
    </message>
    <message>
        <location filename="../logadvisor.cpp" line="55"/>
        <source>Not enough memory for the requested settings. Lower the context size or the number of GPU layers, use a quantized KV cache, or keep MoE experts on the CPU.</source>
        <translation>Недостатньо пам&apos;яті для вибраних налаштувань. Зменште розмір контексту або кількість шарів на GPU, використайте квантований KV-кеш або залиште MoE-експертів на CPU.</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../mainwindow.ui" line="14"/>
        <location filename="../mainwindow.cpp" line="413"/>
        <location filename="../mainwindow.cpp" line="966"/>
        <location filename="../mainwindow.cpp" line="974"/>
        <location filename="../mainwindow.cpp" line="986"/>
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
        <location filename="../mainwindow.ui" line="475"/>
        <location filename="../mainwindow.ui" line="499"/>
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
        <location filename="../mainwindow.ui" line="658"/>
        <location filename="../mainwindow.ui" line="711"/>
        <location filename="../mainwindow.ui" line="744"/>
        <location filename="../mainwindow.ui" line="774"/>
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
        <location filename="../mainwindow.ui" line="779"/>
        <source>On</source>
        <translation>Увімк.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="267"/>
        <location filename="../mainwindow.ui" line="784"/>
        <source>Off</source>
        <translation>Вимк.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="529"/>
        <source>Advanced</source>
        <translation>Додатково</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1102"/>
        <source>Extra Arguments:</source>
        <translation>Додаткові аргументи:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1112"/>
        <source>Additional llama-server arguments, appended as-is. Quotes are supported.</source>
        <translation>Додаткові аргументи llama-server, додаються як є. Лапки підтримуються.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1070"/>
        <source>Context Shift</source>
        <translation>Зсув контексту (Context Shift)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="395"/>
        <source>Listen on all network interfaces, IPv4 and IPv6 (--host ::), so other computers on your network can use the server.</source>
        <translation>Слухати на всіх мережевих інтерфейсах, IPv4 та IPv6 (--host ::), щоб інші комп’ютери у вашій мережі могли користуватися сервером.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="431"/>
        <source>Encryption:</source>
        <translation>Шифрування:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="438"/>
        <source>Use HTTPS (encrypt traffic)</source>
        <translation>Використовувати HTTPS (шифрувати трафік)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="441"/>
        <location filename="../mainwindow.cpp" line="1615"/>
        <source>Serve over HTTPS so prompts, answers and the API key cannot be read on the network (--ssl-cert-file / --ssl-key-file).</source>
        <translation>Працювати через HTTPS, щоб запити, відповіді та API-ключ не можна було прочитати в мережі (--ssl-cert-file / --ssl-key-file).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="448"/>
        <source>New Certificate</source>
        <translation>Новий сертифікат</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="451"/>
        <source>Issue a new server certificate for this computer&apos;s current name and addresses. The local CA is kept, so devices that already trust it keep working.</source>
        <translation>Випустити новий сертифікат сервера для поточного імені та адрес цього комп’ютера. Локальний CA зберігається, тож пристрої, які вже йому довіряють, і далі працюватимуть.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="458"/>
        <source>Certificate:</source>
        <translation>Сертифікат:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="465"/>
        <location filename="../mainwindow.ui" line="489"/>
        <source>created by the launcher</source>
        <translation>створюється лаунчером</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="468"/>
        <source>PEM certificate (chain) to use instead of the one the launcher creates. Leave empty to let the launcher manage it.</source>
        <translation>PEM-сертифікат (ланцюжок) замість створеного лаунчером. Залиште порожнім, щоб лаунчер керував ним сам.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="482"/>
        <source>Private key:</source>
        <translation>Приватний ключ:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="492"/>
        <source>PEM private key of your own certificate.</source>
        <translation>Приватний ключ PEM вашого власного сертифіката.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="560"/>
        <source>Memory and performance</source>
        <translation>Пам’ять і продуктивність</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="566"/>
        <source>KV cache type:</source>
        <translation>Тип KV-кешу:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="573"/>
        <source>Data type of the context (KV) cache, passed as -ctk/-ctv. q8_0 halves its memory use with almost no quality loss; q4_0 quarters it. A quantized cache needs Flash Attention.</source>
        <translation>Тип даних кешу контексту (KV), передається як -ctk/-ctv. q8_0 удвічі зменшує його розмір майже без втрати якості; q4_0 — учетверо. Квантований кеш потребує Flash Attention.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="577"/>
        <source>Default (f16)</source>
        <translation>Типовий (f16)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="582"/>
        <source>q8_0</source>
        <translation>q8_0</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="587"/>
        <source>q4_0</source>
        <translation>q4_0</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="605"/>
        <source>Keep KV cache in system RAM (--no-kv-offload)</source>
        <translation>Тримати KV-кеш в оперативній пам’яті (--no-kv-offload)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="608"/>
        <source>Saves VRAM for model layers at the cost of slower generation.</source>
        <translation>Звільняє відеопам’ять для шарів моделі ціною повільнішої генерації.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="615"/>
        <source>Lock model in RAM (--mlock)</source>
        <translation>Закріпити модель у пам’яті (--mlock)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="618"/>
        <source>Prevent the system from swapping the model out.</source>
        <translation>Не дозволяти системі вивантажувати модель у своп.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="625"/>
        <source>Load model fully instead of mapping it (--no-mmap)</source>
        <translation>Завантажувати модель повністю, без відображення файлу (--no-mmap)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="628"/>
        <source>Read the whole model into memory at start instead of memory-mapping the file.</source>
        <translation>Прочитати всю модель у пам’ять під час запуску замість відображення файлу в пам’ять.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="635"/>
        <source>CPU threads:</source>
        <translation>Потоки CPU:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="651"/>
        <source>Number of CPU threads used for generation (-t).</source>
        <translation>Кількість потоків CPU для генерації (-t).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="668"/>
        <source>Batch / micro-batch:</source>
        <translation>Батч / мікробатч:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="686"/>
        <source>Logical batch size (-b): how many prompt tokens are submitted at once.</source>
        <translation>Логічний розмір батча (-b): скільки токенів промпту подається за раз.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="702"/>
        <source>Physical batch size (-ub). Larger values speed up prompt processing but need more VRAM.</source>
        <translation>Фізичний розмір батча (-ub). Більші значення пришвидшують обробку промпту, але потребують більше відеопам’яті.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="721"/>
        <source>Parallel slots:</source>
        <translation>Паралельні слоти:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="737"/>
        <source>Number of requests served at the same time (-np). The context is split between the slots.</source>
        <translation>Скільки запитів обслуговується одночасно (-np). Контекст ділиться між слотами.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="757"/>
        <source>Chat</source>
        <translation>Чат</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="763"/>
        <source>Thinking:</source>
        <translation>Міркування:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="770"/>
        <source>Whether reasoning models think before answering (--reasoning). Auto follows the chat template.</source>
        <translation>Чи міркують моделі перед відповіддю (--reasoning). «Авто» — як визначає шаблон чату.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="792"/>
        <source>Thinking budget:</source>
        <translation>Бюджет міркувань:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="808"/>
        <source>Maximum number of thinking tokens (--reasoning-budget).</source>
        <translation>Максимальна кількість токенів на міркування (--reasoning-budget).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="815"/>
        <source>Unlimited</source>
        <translation>Без обмежень</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="825"/>
        <source>Jinja chat template</source>
        <translation>Шаблон чату Jinja</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="828"/>
        <source>Use the chat template embedded in the model (needed for tool calling). Unchecked passes --no-jinja.</source>
        <translation>Використовувати шаблон чату, вбудований у модель (потрібен для виклику інструментів). Якщо вимкнено, передається --no-jinja.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="838"/>
        <source>Vision:</source>
        <translation>Зір:</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="845"/>
        <source>Load multimodal projector</source>
        <translation>Завантажити мультимодальний проєктор</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="848"/>
        <source>Load the mmproj*.gguf file found next to the model, so the model can see images (--mmproj).</source>
        <translation>Завантажити файл mmproj*.gguf, що лежить поруч із моделлю, щоб модель могла бачити зображення (--mmproj).</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="862"/>
        <source>Keep projector on CPU (--no-mmproj-offload)</source>
        <translation>Тримати проєктор на CPU (--no-mmproj-offload)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="865"/>
        <source>Saves VRAM; image processing becomes slower.</source>
        <translation>Економить відеопам’ять; обробка зображень стає повільнішою.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="875"/>
        <source>Default sampling</source>
        <translation>Семплінг за замовчуванням</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="881"/>
        <source>Checked values replace the server defaults; clients can still override them per request.</source>
        <translation>Позначені значення замінюють типові значення сервера; клієнти все одно можуть змінити їх у кожному запиті.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="891"/>
        <source>Temperature</source>
        <translation>Температура</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="924"/>
        <source>Top P</source>
        <translation>Top P</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="950"/>
        <source>Top K</source>
        <translation>Top K</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="977"/>
        <source>Min P</source>
        <translation>Min P</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1003"/>
        <source>Presence penalty</source>
        <translation>Штраф за присутність</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1006"/>
        <source>Penalizes tokens that already appeared, regardless of how often (--presence-penalty). llama.cpp only looks at the last --repeat-last-n tokens (64 by default). Leave off for code, SQL or JSON output: it distorts column names and syntax.</source>
        <translation>Штрафує токени, які вже з&apos;являлися, незалежно від того, скільки разів (--presence-penalty). llama.cpp враховує лише останні --repeat-last-n токенів (типово 64). Не вмикайте для генерації коду, SQL чи JSON: штраф спотворює назви колонок і синтаксис.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1032"/>
        <source>Frequency penalty</source>
        <translation>Штраф за частоту</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1035"/>
        <source>Penalizes tokens in proportion to how often they already appeared (--frequency-penalty). llama.cpp only looks at the last --repeat-last-n tokens (64 by default). Leave off for code, SQL or JSON output: it distorts column names and syntax.</source>
        <translation>Штрафує токени пропорційно до того, скільки разів вони вже з&apos;являлися (--frequency-penalty). llama.cpp враховує лише останні --repeat-last-n токенів (типово 64). Не вмикайте для генерації коду, SQL чи JSON: штраф спотворює назви колонок і синтаксис.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1064"/>
        <source>Other</source>
        <translation>Інше</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1077"/>
        <source>Prometheus metrics endpoint (--metrics)</source>
        <translation>Метрики Prometheus (--metrics)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1080"/>
        <source>Expose /metrics with token counters and throughput.</source>
        <translation>Відкрити /metrics з лічильниками токенів і швидкістю.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1087"/>
        <source>Skip warm-up run (--no-warmup)</source>
        <translation>Без прогрівального запуску (--no-warmup)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1090"/>
        <source>Start faster by skipping the empty warm-up pass.</source>
        <translation>Швидший старт без порожнього прогрівального проходу.</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1109"/>
        <source>e.g. --seed 42 --repeat-penalty 1.1</source>
        <translation>напр. --seed 42 --repeat-penalty 1.1</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1133"/>
        <source>Monitor</source>
        <translation>Монітор</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1175"/>
        <source>Log</source>
        <translation>Журнал</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1213"/>
        <source>Copy Log</source>
        <translation>Копіювати журнал</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1220"/>
        <source>Clear</source>
        <translation>Очистити</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1230"/>
        <source>Launcher</source>
        <translation>Запуск програми</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1236"/>
        <source>Launch at login (in tray)</source>
        <translation>Запускати при вході (у треї)</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1243"/>
        <source>Start server when the launcher opens</source>
        <translation>Запускати сервер разом із програмою</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1250"/>
        <source>Notifications</source>
        <translation>Сповіщення</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1289"/>
        <location filename="../mainwindow.cpp" line="406"/>
        <source>Open in Browser</source>
        <translation>Відкрити в браузері</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1296"/>
        <source>Copy URL</source>
        <translation>Копіювати адресу</translation>
    </message>
    <message>
        <location filename="../mainwindow.ui" line="1197"/>
        <source>Console output will appear here...</source>
        <translation>Тут з’явиться вивід сервера...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="99"/>
        <source>the program could not be started (file not found or not executable)</source>
        <translation>програму не вдалося запустити (файл не знайдено або він не виконуваний)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="101"/>
        <source>the process crashed</source>
        <translation>процес аварійно завершився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="103"/>
        <source>operation timed out</source>
        <translation>вичерпано час очікування</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="105"/>
        <source>write error</source>
        <translation>помилка запису</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="107"/>
        <source>read error</source>
        <translation>помилка читання</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="109"/>
        <source>unknown error</source>
        <translation>невідома помилка</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="191"/>
        <source>Server did not stop in time, killing it.</source>
        <translation>Сервер не зупинився вчасно — примусове завершення.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="404"/>
        <location filename="../mainwindow.cpp" line="457"/>
        <location filename="../mainwindow.cpp" line="634"/>
        <source>Start Server</source>
        <translation>Запустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="405"/>
        <location filename="../mainwindow.cpp" line="459"/>
        <location filename="../mainwindow.cpp" line="634"/>
        <source>Stop Server</source>
        <translation>Зупинити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="408"/>
        <source>Show/Restore</source>
        <translation>Показати/сховати</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="410"/>
        <source>Exit</source>
        <translation>Вийти</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="604"/>
        <source>Stopped</source>
        <translation>Зупинено</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="605"/>
        <source>Starting</source>
        <translation>Запуск</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="389"/>
        <location filename="../mainwindow.cpp" line="606"/>
        <source>Loading model</source>
        <translation>Завантаження моделі</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="283"/>
        <source>%1 GiB</source>
        <translation>%1 ГіБ</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="284"/>
        <source>%1 tok/s</source>
        <translation>%1 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="286"/>
        <source>CPU</source>
        <translation>CPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="287"/>
        <location filename="../mainwindow.cpp" line="292"/>
        <source>System</source>
        <translation>Система</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="291"/>
        <source>RAM</source>
        <translation>ОЗП</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="295"/>
        <source>GPU load</source>
        <translation>Завантаження GPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="299"/>
        <source>VRAM</source>
        <translation>Відеопам’ять</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="302"/>
        <source>Generation speed</source>
        <translation>Швидкість генерації</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="308"/>
        <location filename="../mainwindow.cpp" line="379"/>
        <source>The server is not running.</source>
        <translation>Сервер не запущено.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="350"/>
        <source>sleeping (power saving)</source>
        <translation>сплячий режим (енергозбереження)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="355"/>
        <source>%1 W</source>
        <translation>%1 Вт</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="357"/>
        <source>%1 MHz</source>
        <translation>%1 МГц</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="364"/>
        <source>Shared system memory (GTT): %1 GiB</source>
        <translation>Спільна системна пам’ять (GTT): %1 ГіБ</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="373"/>
        <source>generation %1 tok/s</source>
        <translation>генерація %1 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="375"/>
        <source>prompt %1 tok/s</source>
        <translation>промпт %1 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="376"/>
        <source>Last request: %1</source>
        <translation>Останній запит: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="384"/>
        <source>Reading prompt: &lt;b&gt;%1 %&lt;/b&gt; at %2 tok/s</source>
        <translation>Обробка промпту: &lt;b&gt;%1 %&lt;/b&gt;, %2 ток/с</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="387"/>
        <source>Generating: &lt;b&gt;%1 tok/s&lt;/b&gt;</source>
        <translation>Генерація: &lt;b&gt;%1 ток/с&lt;/b&gt;</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="389"/>
        <source>Idle</source>
        <translation>Очікує запитів</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="391"/>
        <source>Context: %1 / %2 (%3 %)</source>
        <translation>Контекст: %1 / %2 (%3 %)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="394"/>
        <source>llama-server uses %1 GiB RAM</source>
        <translation>llama-server займає %1 ГіБ ОЗП</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="461"/>
        <source>Restart Server</source>
        <translation>Перезапустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="464"/>
        <source>Restart Launcher (Server Keeps Running)</source>
        <translation>Перезапустити лаунчер (сервер працює далі)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="466"/>
        <source>Quit</source>
        <translation>Вийти</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="475"/>
        <source>Menu</source>
        <translation>Меню</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="498"/>
        <source>Quit Llama Launcher</source>
        <translation>Вихід з Llama Launcher</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="607"/>
        <source>Running</source>
        <translation>Працює</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="619"/>
        <source>network:</source>
        <translation>мережа:</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="645"/>
        <source>Llama Launcher
Status: %1</source>
        <translation>Llama Launcher
Стан: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="647"/>
        <source>
Model: %1
%2</source>
        <translation>
Модель: %1
%2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="655"/>
        <source>Llama server is ready</source>
        <translation>Сервер готовий</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="656"/>
        <source>%1
%2</source>
        <translation>%1
%2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="673"/>
        <source>Copied %1</source>
        <translation>Скопійовано %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="698"/>
        <source>no network connection found</source>
        <translation>мережеве з’єднання не знайдено</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="700"/>
        <source>Other computers can connect at %1</source>
        <translation>Інші комп’ютери можуть підключатися за адресою %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="702"/>
        <source>No API key: anyone on the network can use the server. Set a key, especially on public Wi-Fi.</source>
        <translation>Ключа API немає: будь-хто в мережі зможе користуватися сервером. Задайте ключ, особливо в публічних Wi-Fi.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="710"/>
        <source>A certificate will be created when the server starts.</source>
        <translation>Сертифікат буде створено під час запуску сервера.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="714"/>
        <source>To avoid browser warnings on other devices, import the CA certificate %1 there once and trust it for websites.</source>
        <translation>Щоб на інших пристроях не було попереджень браузера, один раз імпортуйте туди сертифікат CA %1 і дозвольте йому довіру для вебсайтів.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="719"/>
        <source>The server certificate will be renewed at start (%1).</source>
        <translation>Сертифікат сервера буде оновлено під час запуску (%1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="722"/>
        <source>Choose both the certificate and its private key, or clear both to let the launcher create them.</source>
        <translation>Виберіть і сертифікат, і його приватний ключ, або очистьте обидва поля, щоб лаунчер створив їх сам.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="726"/>
        <source>Qt has no TLS support here, so the launcher cannot check the server&apos;s status.</source>
        <translation>Qt тут не підтримує TLS, тому лаунчер не зможе перевіряти стан сервера.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="764"/>
        <source>Created a new server certificate (%1) for %2 in %3</source>
        <translation>Створено новий сертифікат сервера (%1) для %2 у %3</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="772"/>
        <source>choose both the certificate and its private key, or clear both to let the launcher create them.</source>
        <translation>виберіть і сертифікат, і його приватний ключ, або очистьте обидва поля, щоб лаунчер створив їх сам.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="776"/>
        <source>%1 is not a readable PEM certificate.</source>
        <translation>%1 не є читабельним PEM-сертифікатом.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="780"/>
        <source>cannot read the private key %1.</source>
        <translation>не вдається прочитати приватний ключ %1.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="539"/>
        <location filename="../mainwindow.cpp" line="805"/>
        <location filename="../mainwindow.cpp" line="964"/>
        <location filename="../mainwindow.cpp" line="972"/>
        <location filename="../mainwindow.cpp" line="984"/>
        <location filename="../mainwindow.cpp" line="1656"/>
        <source>Warning</source>
        <translation>Попередження</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="498"/>
        <source>The server is running.</source>
        <translation>Сервер працює.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="500"/>
        <source>Stop it, or keep it running: the launcher picks it up again the next time it starts.</source>
        <translation>Зупиніть його або залиште працювати: лаунчер підхопить його під час наступного запуску.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="501"/>
        <source>Keep Server Running</source>
        <translation>Залишити сервер працювати</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="502"/>
        <source>Stop Server and Quit</source>
        <translation>Зупинити сервер і вийти</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="539"/>
        <source>Could not start %1.</source>
        <translation>Не вдалося запустити %1.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="805"/>
        <source>Could not create the certificate: %1</source>
        <translation>Не вдалося створити сертифікат: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="808"/>
        <source>Created a certificate for %1</source>
        <translation>Створено сертифікат для %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="814"/>
        <source>Select certificate</source>
        <translation>Виберіть сертифікат</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="815"/>
        <source>PEM files (*.pem *.crt *.cer);;All files (*)</source>
        <translation>Файли PEM (*.pem *.crt *.cer);;Усі файли (*)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="822"/>
        <source>Select private key</source>
        <translation>Виберіть приватний ключ</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="824"/>
        <source>PEM files (*.pem *.key);;All files (*)</source>
        <translation>Файли PEM (*.pem *.key);;Усі файли (*)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="964"/>
        <location filename="../mainwindow.cpp" line="966"/>
        <source>Please select a model first.</source>
        <translation>Спочатку виберіть модель.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="970"/>
        <source>Model file not found:
%1</source>
        <translation>Файл моделі не знайдено:
%1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="982"/>
        <source>HTTPS: %1</source>
        <translation>HTTPS: %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1027"/>
        <source>Starting %1 %2</source>
        <translation>Запуск %1 %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1056"/>
        <source>Picked up the server that was already running (started %1).</source>
        <translation>Підхоплено сервер, що вже працював (запущено %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1084"/>
        <source>Stopping server...</source>
        <translation>Зупинка сервера...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1118"/>
        <source>changed, takes effect after a restart</source>
        <translation>змінено, діятиме після перезапуску</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1127"/>
        <source>Restart the server</source>
        <translation>Перезапустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1127"/>
        <source>Start the server</source>
        <translation>Запустити сервер</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1135"/>
        <source>Set GPU layers to Auto</source>
        <translation>Увімкнути «Авто» для шарів GPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1136"/>
        <source>Turn off Context Shift</source>
        <translation>Вимкнути Context Shift</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1184"/>
        <source>Log copied to clipboard</source>
        <translation>Журнал скопійовано в буфер обміну</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="994"/>
        <location filename="../mainwindow.cpp" line="1031"/>
        <source>Could not start %1: %2</source>
        <translation>Не вдалося запустити %1: %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="997"/>
        <location filename="../mainwindow.cpp" line="1034"/>
        <source>Llama server failed to start</source>
        <translation>Сервер не запустився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1199"/>
        <source>Server stopped (signal %1).</source>
        <translation>Сервер зупинено (сигнал %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1200"/>
        <source>Server crashed (signal %1).</source>
        <translation>Сервер аварійно завершився (сигнал %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1202"/>
        <source>Server stopped (exit code %1).</source>
        <translation>Сервер зупинено (код виходу %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1204"/>
        <source>Server stopped.</source>
        <translation>Сервер зупинено.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1212"/>
        <source>Server stopped unexpectedly (exit code %1).</source>
        <translation>Сервер несподівано зупинився (код виходу %1).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1215"/>
        <source>Llama server stopped</source>
        <translation>Сервер зупинився</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1256"/>
        <source>Select llama-server</source>
        <translation>Виберіть llama-server</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1309"/>
        <source>Checking %1...</source>
        <translation>Перевірка %1...</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1316"/>
        <source>%1 &amp;mdash; version %2</source>
        <translation>%1 &amp;mdash; версія %2</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1320"/>
        <source>Cannot run %1</source>
        <translation>Не вдається запустити %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1333"/>
        <source>All GPUs (auto)</source>
        <translation>Усі GPU (авто)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1349"/>
        <source>CPU only</source>
        <translation>Лише CPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1363"/>
        <source>%1 (not detected)</source>
        <translation>%1 (не знайдено)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1378"/>
        <source>Select Model</source>
        <translation>Виберіть модель</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1379"/>
        <source>GGUF Models (*.gguf);;All Files (*)</source>
        <translation>Моделі GGUF (*.gguf);;Усі файли (*)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1388"/>
        <source>Select Models Folder</source>
        <translation>Виберіть теку моделей</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1446"/>
        <source>(missing)</source>
        <translation>(відсутня)</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1476"/>
        <source>No mmproj*.gguf file next to the model.</source>
        <translation>Поруч із моделлю немає файлу mmproj*.gguf.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1480"/>
        <source>No model selected.</source>
        <translation>Модель не вибрано.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1489"/>
        <source>Cannot read model metadata: %1</source>
        <translation>Не вдалося прочитати метадані моделі: %1</translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="1498"/>
        <source>%n parts</source>
        <translation>
            <numerusform>%n частина</numerusform>
            <numerusform>%n частини</numerusform>
            <numerusform>%n частин</numerusform>
        </translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="1500"/>
        <source>%n layers</source>
        <translation>
            <numerusform>%n шар</numerusform>
            <numerusform>%n шари</numerusform>
            <numerusform>%n шарів</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1502"/>
        <source>native context %1</source>
        <translation>рідний контекст %1</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1504"/>
        <source>MoE: %1 experts, %2 active</source>
        <translation>MoE: %1 експертів, %2 активних</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1528"/>
        <source>all GPUs</source>
        <translation>усіх GPU</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1542"/>
        <source>Model (%1) fits into free VRAM on %2 (%3).</source>
        <translation>Модель (%1) вміщується у вільну відеопам’ять %2 (%3).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1546"/>
        <source>Model (%1) is larger than free VRAM on %2 (%3): part of it will run on the CPU, which is slower.</source>
        <translation>Модель (%1) більша за вільну відеопам’ять %2 (%3): частина виконуватиметься на CPU, а це повільніше.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1549"/>
        <source>For this MoE model, keeping the experts on the CPU (option below) usually gives the best speed.</source>
        <translation>Для цієї MoE-моделі найкращу швидкість зазвичай дає розміщення експертів на CPU (опція нижче).</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1588"/>
        <source>A quantized KV cache does not work with Flash Attention forced off. Set Flash Attention to Auto or On on the Model tab; the server decides by itself whether your GPU can use it.</source>
        <translation>Квантований KV-кеш не працює, якщо Flash Attention примусово вимкнено. Встановіть для Flash Attention «Авто» або «Увімк.» на вкладці «Модель»: сервер сам визначить, чи може ваш GPU його використати.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1613"/>
        <source>Always on while access from the local network is allowed: nothing leaves this computer unencrypted.</source>
        <translation>Завжди ввімкнено, коли дозволено доступ із локальної мережі: ніщо не залишає цей комп’ютер незашифрованим.</translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1656"/>
        <source>Could not write %1</source>
        <translation>Не вдалося записати %1</translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="1877"/>
        <source>Moved %n option(s) from Extra Arguments to the Advanced tab</source>
        <translation>
            <numerusform>Перенесено %n опцію з додаткових аргументів на вкладку «Додатково»</numerusform>
            <numerusform>Перенесено %n опції з додаткових аргументів на вкладку «Додатково»</numerusform>
            <numerusform>Перенесено %n опцій з додаткових аргументів на вкладку «Додатково»</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../mainwindow.cpp" line="1113"/>
        <source>Hints from the server log:</source>
        <translation>Підказки з журналу сервера:</translation>
    </message>
    <message numerus="yes">
        <location filename="../mainwindow.cpp" line="1099"/>
        <source>The server log has %n hint(s), see the Log tab</source>
        <translation>
            <numerusform>У журналі сервера %n підказка, див. вкладку «Журнал»</numerusform>
            <numerusform>У журналі сервера %n підказки, див. вкладку «Журнал»</numerusform>
            <numerusform>У журналі сервера %n підказок, див. вкладку «Журнал»</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>ServerProcess</name>
    <message>
        <location filename="../serverprocess.cpp" line="112"/>
        <source>the server is already running</source>
        <translation>сервер уже працює</translation>
    </message>
    <message>
        <location filename="../serverprocess.cpp" line="116"/>
        <source>cannot create %1</source>
        <translation>не вдалося створити %1</translation>
    </message>
    <message>
        <location filename="../serverprocess.cpp" line="121"/>
        <source>cannot write %1: %2</source>
        <translation>не вдалося записати %1: %2</translation>
    </message>
</context>
<context>
    <name>TlsCert</name>
    <message>
        <location filename="../tlscert.cpp" line="44"/>
        <source>openssl did not finish: %1</source>
        <translation>openssl не завершився: %1</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="48"/>
        <source>openssl failed: %1</source>
        <translation>помилка openssl: %1</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="58"/>
        <source>Cannot write %1: %2</source>
        <translation>Не вдається записати %1: %2</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="160"/>
        <source>Cannot read the generated certificate.</source>
        <translation>Не вдається прочитати створений сертифікат.</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="171"/>
        <source>Cannot write %1</source>
        <translation>Не вдається записати %1</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="226"/>
        <location filename="../tlscert.cpp" line="230"/>
        <source>no certificate yet</source>
        <translation>сертифіката ще немає</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="232"/>
        <source>issued by a different CA</source>
        <translation>виданий іншим CA</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="236"/>
        <source>expires %1</source>
        <translation>спливає %1</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="243"/>
        <location filename="../tlscert.cpp" line="246"/>
        <source>does not cover %1</source>
        <translation>не охоплює %1</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="253"/>
        <source>The openssl program was not found. Install the openssl package.</source>
        <translation>Не знайдено програму openssl. Встановіть пакет openssl.</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="257"/>
        <source>Cannot create %1</source>
        <translation>Не вдається створити %1</translation>
    </message>
    <message>
        <location filename="../tlscert.cpp" line="264"/>
        <source>Cannot create a temporary folder: %1</source>
        <translation>Не вдається створити тимчасову теку: %1</translation>
    </message>
</context>
<context>
    <name>main</name>
    <message>
        <location filename="../main.cpp" line="112"/>
        <source>Start hidden in the system tray (used at login).</source>
        <translation>Запуститися прихованим у треї (використовується при вході).</translation>
    </message>
    <message>
        <location filename="../main.cpp" line="115"/>
        <source>Wait for the launcher with this process ID to exit first.</source>
        <translation>Спершу дочекатися завершення лаунчера з цим ідентифікатором процесу.</translation>
    </message>
</context>
</TS>
