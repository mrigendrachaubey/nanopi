# 安装编译环境

建议主机使用ubuntu16.04, 安装以下包

```
sudo apt-get install cmake
sudo apt-get install gcc-aarch64-linux-gnu
sudo apt-get install g++-aarch64-linux-gnu

```

# 编译

使用如下命令编译:

```
mkdir build install
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX="../install"
make
make install
```

编译完之后可执行程序和依赖库将安装到`install/rkssddemo`目录

# 安装到设备

- 首先将`ssd`文件夹拷贝进刚才生成的`install/rkssddemo`目录

```
cp -r ssd install/rkssddemo/
```

- 然后将`install/rkssddemo`目录通过scp或者u盘等方式拷贝至rk3399设备

# 运行

```
cd rkssddemo
./rkssddemo -i ssd/test.jpg -o out.jpg -l ssd/coco_labels_list.txt -b ssd/box_priors.txt -g librkssd.so -p ssd/ssd300_91c_param.rkl -n 91
```

