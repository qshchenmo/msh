`msh` 是一款轻量的交互式命令行工具，可以用于为其他应用的提供命令行解析与分流功能.

**只支持Linux系统.**

## 目录
- [安装](#安装)
- [特性](#特性)
- [编程指南](#编程指南)

## 安装

### 准备工作

- gcc 

推荐使用 gcc 4.8.5 以上版本, 用来编译源代码

- libcurses

```
# for Ubuntu
apt-get install libncurses5-dev

# for CentOS
yum install ncurses-devel
```

### 编译

下载源代码
```
git clone https://github.com/qshchenmo/msh.git
```

编译安装
```bash
cd msh

make 

make install
```

> 执行以上步骤后，生成的可执行程序`mshd`将拷贝到 /usr/local/bin 路径

### 启动进程

```
mshd
```

## 特性

### 内部命令支持

`msh`支持以下内部命令

 - **version**
  显示软件版本
 - **quit**
  退出`msh`

### 外部命令支持

可以为`msh`添加外部命令，外部命令以插件的形式运行在`msh`进程上下文

```bash
cd test/
make
```

> 以上操作后，生成的命令行插件 `test.cli` 将被拷贝到 **/etc/msh/external** 目录，`msh`启动时会扫描这个目录加载这个命令插件 

外部命令插件编程方法请参考

### 快捷键

 - **上下方向键** 
  恢复历史输入
 - **Ctrl + C**
   撤销当前行输入
 - **Tab**
   命令关键字补全/提示


## 编程指南

### 定义一条命令行

#### 命令行组成

命令行由 若干个**关键字**(keyword) + 若干**选项**(opt) 组成

比如下面这条命令中

> test1 [opt0 string] [opt1 interger]

**test1** 为唯一的关键字， 而 **opt0** 和 **opt1** 为两个选项

#### 接口定义

```c
/*
 *  创建一个命令定义上下文
 */
void* cmd_ctx_create(void);

/*
 *  销毁一个命令定义上下文
 *  ctx: 上下文
 */
void cmd_ctx_destroy(void* ctx);

/*
 *  定义一个命令行关键字
 *  name: 关键字名
 *  ctx: 上下文
 *  helpstr: 帮助信息
 */
void cmd_def_keyword(char* name, void* ctx, char* helpstr);

/*
 *  定义一个命令行选项
 *  name: 选项名
 *  id: 选项的身份标识
 *  flag: 选项标识
 *  valuehint: 选项取值的提示信息
 *  ctx: 上下文
 *  helpstr: 帮助信息
 */
void cmd_def_option(char* name, int id, int flag, char* valuehint, void* _ctx, char* helpstr)

/*
 *  注册一条命令
 *  ctx: 上下文
 *  handler: 命令解析函数
 */
void cmd_register(void* ctx, cmd_handler handler);

```

#### 例子

```c
static void test1_cmd_install(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("test1", ctx, "test1_helper");    

    cmd_def_option("opt0", OPT0, 0, "string", ctx, "this is opt0's helpstr");

    cmd_def_option("opt1", OPT1, 0, "interger", ctx, "this is opt1's helpstr");

    cmd_register(ctx, test1_cmd_handler);

    cmd_ctx_destroy(ctx);

    return;
}
```

### 解析一条命令行

#### 接口定义

```c
/*
 *  获取选项的值(字符串)
 *  para: 遍历指针
 */
char* cmd_getpara_string(void* para);

/*
 *  获取选项的值(整数)
 *  para: 遍历指针
 */
int cmd_getpara_interger(void* para);

/*
 *  遍历选项
 */
#define CMD_OPT_SCAN(ctx ,id, para) \
    for(para = cmd_getpara(ctx, &(id)); \
        NULL != para; \
        para = cmd_getpara(ctx, &(id)))
```

#### 例子

```c
static int test1_cmd_handler(void* ctx)
{
    void* para = NULL;
    int id = MSH_INVALID_ID;

    CMD_OPT_SCAN(ctx ,id, para)
    {
        switch(id)
        {
            case OPT0:
            {
                printf("opt0 %s\n", cmd_getpara_string(para));
                break;
            }
            case OPT1:
            {
                printf("opt1 %u\n", cmd_getpara_interger(para));
                break;
            }
            default:
            {
                return MSH_USER_ERROR_FAILED;
            }
        }
    }
    
    return MSH_USER_ERROR_SUCCESS;
}
```

#### 效果

```bash
msh # 
  quit     quit msh
  version  show msh version
  test1    test1_helper
msh # tes
msh # test1 
[usage] test1  [opt0 string] [opt1 interger]
 opt0     this is opt0's helpstr
 opt1     this is opt1's helpstr
msh # test1 

```

