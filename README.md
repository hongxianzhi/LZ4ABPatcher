AssetBundle补丁工具，C、C++实现。支持新旧版本文件夹之间的对比，生成补丁，根据补丁和旧版本AssetBundle升级到新版。
1.仅支持解析LZ4格式，format 6。理论上unity5.3到最新的unity2018.1都支持,未细测。
2.合并AssetBundle时采用 LZ4HC_CLEVEL_MIN 级别，提高合并速度；
3.由于AssetBundle格式未公开，部分filegen不能识别，这部分AssetBundle采取二进制比较。
