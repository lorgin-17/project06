build:
	@g++ main.cpp -o main
	@./main

bg:
	@g++ main.cpp -o main
	@./main > /dev/null &
	@echo "демон запущен в фоне"

stop:
	@pkill main
	@echo "демон остановлен"
