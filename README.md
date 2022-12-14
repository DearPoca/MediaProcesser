# MediaRecorder

## libx264配置
```
./configure --prefix=/usr/local --enable-shared
```

## FFmpeg配置
```
./configure --prefix=/usr/local --enable-gpl --enable-shared --disable-static --enable-libx264
```

## libyuv安装
```
cmake ..
make
make install
```