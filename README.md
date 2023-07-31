# contract_compare


## 调用方法
```c++
    xwb::RuntimeConfig config;
    config.block_compare = false; // 是否分块比较
    config.ignore_punctuation = true; // 是否过滤标点符号
    std::string result  = xwb::contract_compare(allOcrResult, config);
```


## 问题
- 分块比较还是联合比较?
  分块比较速度快，但是有可能出现比对错位情况。依赖段落划分的准确性和段落对齐的准确性
  联合比较，不会出现错位。但是速度慢。

  以测试样本16为例：
  分块： 文本比较 1s左右。 错位。
  联合： 文本比较 25s 左右。无错位。

- 表格增删时一边没有坐标
  有一些想法，还没实现。
  可以把表格的比对结果插入到文本比较结果。再结合最后坐标对齐操作实现对齐。

## TODO
- [ ] 优化段落划分
- [ ] 优化段落对齐
- [ ] 表格缺失坐标问题



