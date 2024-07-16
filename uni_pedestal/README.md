# Milkv-duo  C906L 小核 UniProton 支持说明

## MICA 混合部署效果

![](./uniproton.gif)



## 简介

- 适配小核 uniproton 硬件的 duo-buildroot-sdk
- 本工程与上游主线的仓库多加了一个小核的目录  **uniproton**
- 以及修改了小核编译的路径为 **uniproton** 目录，支持duo-buildroot-sdk 的一键出镜像，其中包含的是小核 uniproton 镜像
- 默认按照官方的构建脚本就可以出带 uniproton 的 milkv-duo 镜像

## 约束限制

- 目前只支持 **CV180X** 系列 小核

- 目前只支持 **milkv-duo** 的镜像，如果您要做 duos 等其他开发板的镜像，需要您修改小核为freertos

  - 打开 **path2duobuildroot/build/milkvsetup.sh** 文件

    ```shell
    vim path2duobuildroot/build/milkvsetup.sh
    ```

    

  - 搜索 **FREERTOS_PATH**

    ```bash
    #FREERTOS_PATH="$TOP_DIR"/freertos
     FREERTOS_PATH="$TOP_DIR"/uniproton
    ```

    

  - 取消掉上面的注释，同时添加下面的注释

    ```bash
    FREERTOS_PATH="$TOP_DIR"/freertos
    #FREERTOS_PATH="$TOP_DIR"/uniproton
    ```

## 编译构建方式

- 和官方的出镜像方式保持一致 【请阅读官方的文档，默认 milkvduo 带的是uniproton，非milkvduo 请按照**约束限制**修改小核路径】

## 默认小核做的工作

- 接受 大核的 mailbox 通信，进行点灯测试

- 启动一个线程，隔4S 打印自己的输出信息【建议关闭】

  - 关闭方法

  - 注释 **path2duobuildrootsdk/uniproton/cvitek/task/comm/src/riscv64/comm_main.c** 的第 273 行

  - ```c
    void test_thread(uintptr_t param_1, uintptr_t param_2, uintptr_t param3, uintptr_t param4)
    {
    	
    	while(1) {
    		//printf("[UniProton test Thread]: I'm here!\n");  【这是第273行的内容！！！】
     		PRT_TaskDelay(10);
    	}
    }
    ```



