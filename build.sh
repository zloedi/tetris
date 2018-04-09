rm tetris.zip ; rm tetris.exe ; rm /tmp/tetris.zip ; rm -rf /tmp/tetris ; mkdir /tmp/tetris ; 
make clean && make release_app && cp -r *.exe *.dll data gamecontrollerdb.txt /tmp/tetris/ && cd /tmp && zip -r tetris.zip tetris/* && cd - &&  cp /tmp/tetris.zip ./
