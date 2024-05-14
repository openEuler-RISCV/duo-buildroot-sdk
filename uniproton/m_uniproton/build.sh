#!/bin/bash

RED="\033[31m"
RESET="\033[0m"

SCRIPT_PATH=$(realpath "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")
CURRENT_DIR=$(pwd)

if [ "$SCRIPT_DIR" != "$CURRENT_DIR" ]; then
    echo -e "${RED}must excute build.sh in uniproton root path${RESET}"
    exit 1
fi

normal_exit()
{
	rm -rf "./cmake-3.20.5-linux-x86_64.tar.gz"
}

error_exit()
{
    echo -e "${RED}error occur !${RESET}"
}

trap normal_exit EXIT
trap error_exit ERR
set -e
if [ ! -d "/opt/buildtools" ]; then
    mkdir -p "/opt/buildtools"
    echo "create /opt/buildtools"
else
    echo "/opt/buildtools exsit"
fi

if [ ! -d "./tools" ]; then
    mkdir -p "./tools"
    echo "create tools dir"
else
    echo "tools dir exsit"
fi

# check riscv gcc
if [ ! -d "/opt/buildtools/riscv" ]; then
    if [ ! -d "../../host-tools/gcc/riscv64-elf-x86_64" ]; then
        echo -e "${RED}host-tools don't donloawd${RESET}"
	exit -1
    fi
    cp -rf "../../host-tools/gcc/riscv64-elf-x86_64" /opt/buildtools/riscv
else
    echo "riscv gcc ready"
fi

# check cmake 
if [ ! -d "/opt/buildtools/cmake-3.20.5-linux-x86_64" ]; then
    if [ ! -f "./tools/cmake-3.20.5-linux-x86_64.tar.gz" ]; then
        wget "https://cmake.org/files/v3.20/cmake-3.20.5-linux-x86_64.tar.gz"
	mv cmake-3.20.5-linux-x86_64.tar.gz ./tools
    fi
    tar -xvf "./tools/cmake-3.20.5-linux-x86_64.tar.gz" -C "/opt/buildtools"
else
    echo "cmake-3.20.5 ready"
fi

#check python3 and version
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}Python 3 not exsite${RESET}"
    exit 1
fi

python3_version=$(python3 --version 2>&1 | awk '{print $2}')
# 去除版本号中的所有非数字字符，只保留主版本号和次版本号
version_number=$(echo "$python3_version" | grep -Po '(\d+\.\d+)')

# 检查是否成功提取了版本号
if [[ -z "$version_number" ]]; then
    echo "无法解析Python 3的版本号。"
    exit 1
fi

# 将提取的版本号分割成主版本号和次版本号
IFS='.' read -ra version_parts <<< "$version_number"

# 主版本号
major_version="${version_parts[0]}"
# 次版本号
minor_version="${version_parts[1]}"

# 比较版本号是否大于3.8
if [[ "$major_version" -gt 3 ]] || [[ "$major_version" -eq 3 && "$minor_version" -gt 8 ]]; then
    echo "Python3 > 3.8, check success"
else
    echo -e "${RED}Python3 < 3.8${RESET}"
    exit 1
fi

python3 build.py clean
python3 build.py milkvduol
