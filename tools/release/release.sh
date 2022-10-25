#!/bin/bash

set -e

#
dirty=`git describe --long --tag --dirty --always | awk -F "-" '{print $4}'`
version_full=`git describe --long --tag --dirty --always`
version=`echo $version_full | awk -F "-" '{print $1}'`
version_dev=`echo $version_full | awk -F "-" '{print $2}'`
version_git_rev=`echo $version_full | awk -F "-" '{print $3}'`
if [[ "x$version_dev" != "x" ]]; then
    version=${version}_${version_dev}_${version_git_rev}
fi
if [[ "x$dirty" == "xdirty" ]]; then
    echo -e "\033[33m [WARNING] coding is dirty!!, please commit all code firstly \033[0m"
    git status
fi
echo $version_full
echo $version

release_dir=`pwd`/bin/canmv_$version
rm -rf $release_dir $release_dir/elf
mkdir -p $release_dir
mkdir -p $release_dir/elf

#
cd ../../projects

#
cd canmv_k210
echo "-------------------"
echo "build project canmv_k210"
echo "-------------------"
python project.py distclean
python project.py build
cp build/canmv.bin $release_dir/canmv_$version.bin
cp build/canmv.elf $release_dir/elf/canmv_$version.elf
cd ..

# canmv_k210 with lvgl
cd canmv_k210
echo "-------------------"
echo "build project canmv_k210_with_lvgl"
echo "-------------------"
python project.py distclean
python project.py build --config_file "config_with_lvgl.mk"
cp build/canmv.bin $release_dir/canmv_${version}_with_lvgl.bin
cp build/canmv.elf $release_dir/elf/canmv_${version}_with_lvgl.elf
cd ..

#
cd canmv_k210_minimum
echo "-------------------"
echo "build project canmv_k210_minimum"
echo "-------------------"
python project.py distclean
python project.py build
cp build/canmv.bin $release_dir/canmv_${version}_minimum.bin
cp build/canmv.elf $release_dir/elf/canmv_${version}_minimum.elf
cd ..

# minimum with IDE support
cd canmv_k210_minimum
echo "-------------------"
echo "build project canmv_k210_minimum"
echo "-------------------"
python project.py distclean
python project.py build --config_file "config_with_ide_support.mk"
cp build/canmv.bin $release_dir/canmv_${version}_minimum_with_ide_support.bin
cp build/canmv.elf $release_dir/elf/canmv_${version}_minimum_with_ide_support.elf
cd ..

# minimum with IDE support
cd canmv_k210_minimum
echo "-------------------"
echo "build project canmv_k210_minimum_speech"
echo "-------------------"
python project.py distclean
python project.py build --config_file "config_defaults_speech_with_ide_support.mk"
cp build/canmv.bin $release_dir/canmv_${version}_minimum_speech_with_ide_support.bin
cp build/canmv.elf $release_dir/elf/canmv_${version}_minimum_speech_with_ide_support.elf
cd ..

# minimum with V4 kmodel support
cd canmv_k210_minimum
echo "-------------------"
echo "build project canmv_k210_minimum_v4_kmodel_support"
echo "-------------------"
python project.py distclean
python project.py build --config_file "config_defaults_kmodel_v4_support.mk"
cp build/canmv.bin $release_dir/canmv_${version}_minimum_with_kmodel_v4_support.bin
cp build/canmv.elf $release_dir/elf/canmv_${version}_minimum_with_kmodel_v4_support.elf
cd ..

# openmv with V4 kmodel & IDE support
cd canmv_k210_minimum
echo "-------------------"
echo "build project canmv_k210_openmv_kmodel_v4_with_ide_support"
echo "-------------------"
python project.py distclean
python project.py build --config_file "config_defaults_openmv_kmodel_v4_with_ide_support.mk"
cp build/canmv.bin $release_dir/canmv_${version}_openmv_kmodel_v4_with_ide_support.bin
cp build/canmv.elf $release_dir/elf/canmv_${version}_openmv_kmodel_v4_with_ide_support.elf
cd ..

# add readme.txt
cp ../tools/release/readme.txt $release_dir/readme.txt

cd $release_dir
7z a elf_canmv_${version}.7z elf/*
rm -rf elf

ls -al

