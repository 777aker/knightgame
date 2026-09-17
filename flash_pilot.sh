source ~/zephyrproject/.venv/bin/activate
cd ..
west build -p always -b nucleo_h723zg knightgame/pilot
west flash