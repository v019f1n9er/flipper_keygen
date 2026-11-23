#include <furi.h>
#include <furi_hal.h>
#include <dialogs/dialogs.h>
#include <stdio.h>
#include <storage/storage.h>
#include <string.h>

// границы длины пароля
#define MIN_LENGTH 8
#define MAX_LENGTH 20

// функция генерации пароля
void generate_password(char* buffer, int length) {
    for(int i = 0; i < length; i++) {
        uint32_t ochered = furi_hal_random_get() % 3; // аппаратный рандомайзер

        if(ochered == 0) {
            buffer[i] = (furi_hal_random_get() % 10) + '0'; // 10 цифр (0-9)
        } else if(ochered == 1) {
            buffer[i] = (furi_hal_random_get() % 26) + 'a'; // 26 строчных символов (a-z)
        } else if(ochered == 2) {
            buffer[i] = (furi_hal_random_get() % 26) + 'A'; // 26 заглавных символов (A-Z)
        }
    }
    buffer[length] = '\0'; // символ "стоп", чтобы не вызвать OOM
}

// функция сохранения модифицированного файла пароля
// с возможность последующего ввода
bool save_to_duckyscript(const char* password) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    char script_buffer[64];
    snprintf(script_buffer, sizeof(script_buffer), "DELAY 500\nSTRING %s\n", password);

    bool success =
        storage_file_open(file, "/ext/badusb/generated_key.txt", FSAM_WRITE, FSOM_CREATE_ALWAYS);

    if(success) {
        storage_file_write(file, script_buffer, strlen(script_buffer));
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);

    return success;
}

// основная функция выполнения
int32_t hello_flipper_app(void* p) {
    UNUSED(p);

    int size_string = 12; // стартовый размер генерируемой строки
    char password[MAX_LENGTH + 1]; // размерность буффера
    char header[32];

    generate_password(password, size_string); // вызов функции генерации пароля
    save_to_duckyscript(password); // вызов функции сохранения файла с паролем

    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS); // "ключ" системного сервиса
    DialogMessage* message = dialog_message_alloc(); // выделение памяти под окно

    dialog_message_set_buttons(message, "- Len", "New", "+ Len");

    // функционал для расширенного использования кнопок
    bool running = true;
    while(running == true) {
        snprintf(header, sizeof(header), "Key length: %d", size_string);

        dialog_message_set_header(message, header, 64, 10, AlignCenter, AlignTop);
        dialog_message_set_text(message, password, 64, 30, AlignCenter, AlignTop);

        DialogMessageButton button = dialog_message_show(dialogs, message);

        if(button == DialogMessageButtonLeft) {
            if(size_string > MIN_LENGTH) {
                size_string--;
                generate_password(password, size_string);
            }
        } else if(button == DialogMessageButtonRight) {
            if(size_string < MAX_LENGTH) {
                size_string++;
                generate_password(password, size_string);
            }
        } else if(button == DialogMessageButtonCenter) {
            generate_password(password, size_string);
        } else if(button == DialogMessageButtonBack) {
            save_to_duckyscript(password);
            running = false;
        }
    }

    dialog_message_free(message); // уничтожает бланк и возвращает занятую память обратно системе
    furi_record_close(RECORD_DIALOGS);

    return 0;
}
