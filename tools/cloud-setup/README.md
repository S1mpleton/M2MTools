# Облачная сборка M2MTools Lite

`bootstrap.sh` устанавливает сценарии в `/workspace/.cloud-setup`, скачивает
зависимости в локальный sysroot, собирает приложение и запускает CTest.
Существующие отличающиеся сценарии не перезаписываются автоматически.

```bash
bash /workspace/M2MTools/tools/cloud-setup/bootstrap.sh
source /workspace/.cloud-setup/env.sh
cmake --build /workspace/.cloud-setup/build --parallel 2
ctest --test-dir /workspace/.cloud-setup/build --output-on-failure
python3 /workspace/.cloud-setup/check-startup.py
bash /workspace/.cloud-setup/start.sh
```

`start.sh` запускает приложение из каталога `run`, где хранится отдельная
`variants-lite.db`. Headless-проверки используют Qt `offscreen`.
`check-startup.py` проверяет два запуска, таблицы Lite и целостность SQLite.
Основные тесты — `tests/lite_tests.cpp`; отдельной копии тестов здесь нет.
