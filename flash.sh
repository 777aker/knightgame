source ~/zephyrproject/.venv/bin/activate
cd ..
west build -b nucleo_h723zg knightgame/board1
west flash