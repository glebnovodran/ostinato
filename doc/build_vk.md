```
git clone --depth 1 https://github.com/glebnovodran/ostinato.git
cd ostinato

wget -q -O - https://raw.githubusercontent.com/schaban/crosscore_dev/main/get_vk_headers.sh | sh
wget -q -O - https://raw.githubusercontent.com/schaban/crosscore_dev/main/mk_lib_links.sh | sh
wget -P ext https://raw.githubusercontent.com/schaban/crosscore_dev/main/src/etc/draw/draw_vk_test.cpp
./build.sh -O3 -flto ext/draw_vk_test.cpp -Llib -lvulkan

./run.sh -swap:0 -echo_fps:1 -draw:vk_test
```
