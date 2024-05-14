if [ ! -d "../include" ]; then
  mkdir ../include
fi

if [ ! -d "../libs" ]; then
  mkdir ../libs
fi

pushd ./../../../
python3 build.py rv64virt
cp  output/UniProton/lib/rv64virt/* demos/riscv64virt/libs
cp  output/libboundscheck/lib/rv64virt/* demos/riscv64virt/libs
cp -r src/include/uapi/* demos/riscv64virt/include
cp build/uniproton_config/config_riscv64_rv64virt/prt_buildef.h demos/riscv64virt/include/
popd
