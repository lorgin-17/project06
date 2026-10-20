#include <iostream>
#include <fstream>
#include <string>
#include <cstdio>        // для popen — запуска внешней команды
#include <sys/inotify.h> // для работы с inotify
#include <unistd.h>      // для чтения изменений

using namespace std;

// папка, которую мониторим
string watch_dir = "/home/carvita/projects/project06/watch";
// файл лога (лежит НЕ в отслеживаемой папке,
// иначе запись в лог сама вызывала бы событие изменения)
string log_path = "/home/carvita/projects/project06/watch.log";

// дописывает строку в конец файла лога
void write_log(string str) {
	ofstream ofs(log_path, ios::app); // ios::app — дописывать, а не перезаписывать
	if (ofs.is_open()) {
		ofs << str << endl;
		ofs.close();
	}
}

// прогоняет текст из файла через figlet и возвращает результат
string run_figlet(string path) {
	// команда вида: figlet < "/home/carvita/projects/project06/watch/file.txt"
	// figlet читает текст из файла и рисует его большими буквами
	string command = "figlet < \"" + path + "\"";

	// запускаем команду, "r" — будем читать то, что она выводит
	FILE* pipe = popen(command.c_str(), "r");
	if (pipe == NULL) {
		return "не удалось запустить figlet\n";
	}

	string result;
	char line[256];
	// читаем вывод figlet построчно и добавляем отступ
	while (fgets(line, sizeof(line), pipe) != NULL) {
		result += "    ";
		result += line;
	}
	pclose(pipe); // закрываем команду
	return result;
}

int main(int argc, char* argv[]) {
	// пути можно передать аргументами: ./main /путь/к/папке /путь/к/логу
	if (argc > 1) watch_dir = argv[1];
	if (argc > 2) log_path = argv[2];

	// создаем файловый дескриптор для отслеживания
	int inotify_fd = inotify_init();
	if (inotify_fd < 0) {
		cout << "ошибка inotify_init" << endl;
		return 1;
	}

	// подключаем мониторинг папки на создание, изменение и удаление файлов
	int watch_fd = inotify_add_watch(inotify_fd, watch_dir.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
	if (watch_fd < 0) {
		cout << "не удалось отслеживать папку " << watch_dir << endl;
		return 1;
	}

	cout << "мониторю папку " << watch_dir << ", лог пишу в " << log_path << endl;

	// демон работает бесконечно
	while (true) {
		char buffer[10000]; // буфер под события изменений

		// блокируемся, пока не произойдет какое-нибудь изменение
		int length = read(inotify_fd, buffer, 10000);

		// событий может прийти несколько, обходим все
		int i = 0;
		while (i < length) {
			struct inotify_event *event = (struct inotify_event *)&buffer[i];

			// event->len == 0 значит событие про саму папку, без имени файла
			// и пропускаем скрытые файлы (начинаются с точки) —
			// это временные файлы редакторов, например .test.txt.swp от nano
			if (event->len > 0 && event->name[0] != '.') {
				string name = event->name;

				if (event->mask & IN_CREATE) {
					write_log("---");
					write_log("Файл " + name + " добавлен");
					cout << "создали файл " << name << endl;
				}
				else if (event->mask & IN_MODIFY) {
					// вариант 1: текст из файла прогоняем через figlet
					string art = run_figlet(watch_dir + "/" + name);
					write_log("---");
					write_log("Файл " + name + " изменился, его новое содержимое через figlet:");
					write_log(art);
					cout << "изменился " << name << endl;
				}
				else if (event->mask & IN_DELETE) {
					write_log("---");
					write_log("Файл " + name + " удален");
					cout << "удалили файл " << name << endl;
				}
			}

			// сдвигаемся на следующее событие
			i += sizeof(struct inotify_event) + event->len;
		}
	}

	inotify_rm_watch(inotify_fd, watch_fd);
	close(inotify_fd);
	return 0;
}
