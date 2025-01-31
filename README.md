# RQED

### 说明
记得用/MD编译,不然会报毒

使用了chatgpt生成,deepseek生成,RingQ代码,等。

自己用的免杀工具，如果你用到了，可以点一个star吗(不要看代码，真的很混乱)

### 使用方法

1. shellcode可以使用[EXE2Shellcode.exe](https://github.com/A233S/RQED/raw/refs/heads/main/EXE2Shellcode.exe)将exe转换为shellcode

2. shellcode如果报毒可以使用[Create.exe](https://github.com/A233S/RQED/raw/refs/heads/main/Create.exe)将shellcode简单“加密”
   
   用`-file xxx`输入shellcode文件,`-en`代表编码文件,`-de`代码解码文件,`-ps xxx`代表字符串偏移量（相当于密码）

   例如`Create.exe -file shellcode文件路径 -en -ps 123456`
     
   ~~只是简单编码，不过应该可以让shellcode不报毒~~  

3. 加载shellcode 用Releases里下载的`rqed.exe`加载
     
   例如`rqed.exe -file shellcode文件路径 -ps 密码`,`-ps`参数是选填的,不用`-ps`参数就直接`rqed.exe -file shellcode文件路径`

### 各家杀毒软件的检测情况
![virustotal](https://raw.githubusercontent.com/A233S/RQED/refs/heads/main/QQ20250131-213036.png)  
![微步](https://github.com/A233S/RQED/raw/refs/heads/main/QQ20250131-213422.png)
