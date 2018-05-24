AssetBundle补丁工具，C、C++实现。支持新旧版本文件夹之间的对比，生成补丁，根据补丁和旧版本AssetBundle升级到新版。

1.仅支持解析LZ4格式，format 6。理论上unity5.3到最新的unity2018.1都支持,未细测。

2.合并AssetBundle时采用 LZ4HC_CLEVEL_MIN 级别，提高合并速度；

3.由于AssetBundle格式未公开，部分filegen不能识别，这部分AssetBundle采取二进制比较。

API简介：

/*
设置输出信息回调
MessageHandler callback：回调，通过该回调显示底层传递出的信息数据
*/
set_messagehandler

/*
判断是否为支持的AssetBundle格式
string path：文件路径
*/
assetbundle_check

/*
为AssetBundle生成签名，生成的签名附加在AssetBundle后面
string path：文件路径
*/
assetbundle_sign

/*
开始差异比较工作
string patchfile：patch文件路径，如果文件存在会被覆盖重写。
IntPtr：返回值，初始化成功返回非零值。
*/
assetbundle_diff_init

/*
该函数会比较两个AssetBundle文件的差异并写入patch文件
IntPtr ctx：初始化时的返回值
string path_from：单个旧版本AssetBundle文件路径
string path_to：单个新版本AssetBundle文件路径
string bundlename：bundle文件名，合并时要根据该名字查找旧版本AssetBundle文件，它其实就是旧版本文件名
*/
assetbundle_diff_update

/*
结束比较工作，生成最终patch文件。如果新旧版本没有任何差异，不会有patch文件产生。
IntPtr ctx：初始化时的返回值
*/
assetbundle_diff_final


/*
通过旧版本AssetBundle、patch文件合成新AssetBundle。合并过程中会使用临时文件，新旧目录可以为同一个。
string patch：patch文件路径
string searchpath：旧版本AssetBundle文件夹
string newpath：合并后的文件存放目录
MergeProgress progress：进度回调
*/
assetbundle_merge