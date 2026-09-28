## huayiDF HR/HP系列主控制器与子模块通讯协议

## 说明：

本文档为 huayiDF 嵌入式产品中的 HR/HP产品系列主控制器与子模块通讯协议。

## 正文

通讯帧格式：

<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>数据长度高</td><td rowspan=1 colspan=1>数据长度低</td><td rowspan=1 colspan=1>子模块类型</td><td rowspan=1 colspan=1>数据区1</td><td rowspan=1 colspan=1>数据区2</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>CRC16校验高</td><td rowspan=1 colspan=1>CRC16校验低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

说明：数据长度为数据区长度，不含站号，功能码，子模块类型（02 码除外），CRC 校验

1.1 主站请求子模块报告其软元件资源列表
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>数据长度高</td><td rowspan=1 colspan=1>数据长度低</td><td rowspan=1 colspan=1>子模块类型</td><td rowspan=1 colspan=1>CRC 校验高</td><td rowspan=1 colspan=1>CRC 校验低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x01</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

注：HR21 系列使用此协议没有 子模块类型这一项

1.2 子模块报告软元件资源列表
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>数据长度高</td><td rowspan=1 colspan=1>数据长度低</td><td rowspan=1 colspan=1>数据区1</td><td rowspan=1 colspan=1>数据区2</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>CRC 校验高</td><td rowspan=1 colspan=1>CRC 校验低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x02</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

数据区：数据区内数据排列方式：
<table><tr><td rowspan=1 colspan=1>子模块类型</td><td rowspan=1 colspan=1>子模块输入点数高</td><td rowspan=1 colspan=1>子模块输入点数低</td><td rowspan=1 colspan=1>子模块输出点数高</td><td rowspan=1 colspan=1>子模块输出点数低</td><td rowspan=1 colspan=1>保留</td><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

2.1 主模询问目标模块软元件动态，同时下发需要子模块刷新的操作
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>数据长度高</td><td rowspan=1 colspan=1>数据长度低</td><td rowspan=1 colspan=1>子模块类型</td><td rowspan=1 colspan=1>数据区1</td><td rowspan=1 colspan=1>数据区2</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>CRC 校验高</td><td rowspan=1 colspan=1>CRC 校验低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x03</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

说明：

功能码：0x03，主站请求子模块报告其软元件动态, 同时下发需要子模块刷新的操作

当模块是通讯模块时，子模块类型为包号，分包发送数据。

数据区：

数字量数据区内数据排列方式：

<table><tr><td rowspan=1 colspan=1>软元件类型</td><td rowspan=1 colspan=1>触发事件类型</td><td rowspan=1 colspan=1>触发事件的元件编号高</td><td rowspan=1 colspan=1>触发事件的元件编号低</td><td rowspan=1 colspan=1>元件的旧状态高</td><td rowspan=1 colspan=1>元件的旧状态低</td><td rowspan=1 colspan=1>元件的新状态高</td><td rowspan=1 colspan=1>元件的新状态低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>空</td><td rowspan=1 colspan=1>空</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

模拟量数据区内数据排列方式：

<table><tr><td rowspan=1 colspan=1>软元件类型</td><td rowspan=1 colspan=1>触发事件类型</td><td rowspan=1 colspan=1>触发事件的元件编号高</td><td rowspan=1 colspan=1>触发事件的元件编号低</td><td rowspan=1 colspan=1>通道数据高</td><td rowspan=1 colspan=1>通道数据低</td><td rowspan=1 colspan=1>通道设置参数1</td><td rowspan=1 colspan=1>通道设置参数2</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>通道号</td><td rowspan=1 colspan=1>通道号</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

注意：当模块为交流互感器时通道设置参数 1 为互感器比率设置

通讯模块数据区内数据排列方式：
<table><tr><td rowspan=1 colspan=1>输出数据1</td><td rowspan=1 colspan=1>输出数据2</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>输出数据8</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

2.2 子模块报告软元件动态
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>数据长度高</td><td rowspan=1 colspan=1>数据长度低</td><td rowspan=1 colspan=1>子模块类型</td><td rowspan=1 colspan=1>数据区1</td><td rowspan=1 colspan=1>数据区2</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>CRC 校验高</td><td rowspan=1 colspan=1>CRC 校验低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x04</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

说明：

功能码：0x04，子模块报告软元件资源列表

数据区：

数字量数据区内数据排列方式：

<table><tr><td rowspan=1 colspan=1>软元件类型</td><td rowspan=1 colspan=1>触发事件类型</td><td rowspan=1 colspan=1>触发事件的元件编号高</td><td rowspan=1 colspan=1>触发事件的元件编号低</td><td rowspan=1 colspan=1>元件的旧状态高</td><td rowspan=1 colspan=1>元件的旧状态低</td><td rowspan=1 colspan=1>元件的新状态高</td><td rowspan=1 colspan=1>元件的新状态低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>空</td><td rowspan=1 colspan=1>空</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

模拟量数据区内数据排列方式：
<table><tr><td rowspan=1 colspan=1>软元件类型</td><td rowspan=1 colspan=1>触发事件类型</td><td rowspan=1 colspan=1>触发事件的元件编号高</td><td rowspan=1 colspan=1>触发事件的元件编号低</td><td rowspan=1 colspan=1>通道数据高</td><td rowspan=1 colspan=1>通道数据低</td><td rowspan=1 colspan=1>保留</td><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>通道号</td><td rowspan=1 colspan=1>通道号</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

注：通道号必须从本模块的 0开始顺序向后排，不可以打乱通道号发送数据

通讯模块数据区内数据排列方式：
<table><tr><td rowspan=1 colspan=1>包号 0/1</td><td rowspan=1 colspan=1>输入数据1</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>输入数据63</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

3.1 子站请求报告总模块数量
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>数据长度高</td><td rowspan=1 colspan=1>数据长度低</td><td rowspan=1 colspan=1>数据1</td><td rowspan=1 colspan=1>CRC 校验高</td><td rowspan=1 colspan=1>CRC 校验低</td></tr><tr><td rowspan=1 colspan=1>忽略（任意)</td><td rowspan=1 colspan=1>0x05</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>扩展模块数量</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

说明：  
功能码：0x05，子模块报告全部扩展模块数量

3.2 上位机请求获取总模块信息
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>CRC 校验高</td><td rowspan=1 colspan=1>CRC 校验低</td></tr><tr><td rowspan=1 colspan=1>0X66</td><td rowspan=1 colspan=1>0x55</td><td rowspan=1 colspan=1>0x55</td><td rowspan=1 colspan=1>0x55</td></tr></table>

说明：

功能码：0x055，上位机请求获取总模块数量

3.3 下位机发送模块信息数据
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>总模块数</td><td rowspan=1 colspan=1>模块1识别码</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>模块 N识别码</td><td rowspan=1 colspan=1>CRC 校验高</td><td rowspan=1 colspan=1>CRC 校验低</td></tr><tr><td rowspan=1 colspan=1>0X66</td><td rowspan=1 colspan=1>0x56</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

说明：  
功能码：0x056，下位机发送模块信息数据

## 4.1 主模块下发给子模块配置参数

<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>数据长度高</td><td rowspan=1 colspan=1>数据长度低</td><td rowspan=1 colspan=1>子模块类型(识别码)</td><td rowspan=1 colspan=1>数据区…</td><td rowspan=1 colspan=1>数据区…</td><td rowspan=1 colspan=1>数据区…</td><td rowspan=1 colspan=1>CRC校验高</td><td rowspan=1 colspan=1>CRC校验低</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x06</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td></tr></table>

数字量数据区地址排列：
<table><tr><td rowspan=1 colspan=1>000</td></tr><tr><td rowspan=1 colspan=1>o○o</td></tr><tr><td rowspan=1 colspan=1>oo</td></tr><tr><td rowspan=1 colspan=1>o。</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>通道2滤波常数低</td></tr><tr><td rowspan=1 colspan=1>通道2滤波常数高</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>通道1滤波常数低</td></tr><tr><td rowspan=1 colspan=1>通道1滤波常数高</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>通道0滤波常数低</td></tr><tr><td rowspan=1 colspan=1>通道0滤波常数高</td></tr><tr><td rowspan=1 colspan=1>通道数</td></tr></table>

注：参考文档“HYDF 订货号及通讯识别码 V1.0.3”

## 模拟量数据区地址排列：

<table><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>保留</td></tr><tr><td rowspan=1 colspan=1>通道0几线制选择</td></tr><tr><td rowspan=1 colspan=1>通道0采样类型低</td></tr><tr><td rowspan=1 colspan=1>通道0采样类型高</td></tr><tr><td rowspan=1 colspan=1>通道0校准偏移低</td></tr><tr><td rowspan=1 colspan=1>通道0校准偏移高</td></tr><tr><td rowspan=1 colspan=1>通道0下限报警值低</td></tr><tr><td rowspan=1 colspan=1>通道0下限报警值高</td></tr><tr><td rowspan=1 colspan=1>通道0上限报警值低</td></tr><tr><td rowspan=1 colspan=1>通道0上限报警值高</td></tr><tr><td rowspan=1 colspan=1>通道0采样次数低</td></tr><tr><td rowspan=1 colspan=1>通道0采样次数高</td></tr><tr><td rowspan=1 colspan=1>通道0转换速率低</td></tr><tr><td rowspan=1 colspan=1>通道0转换速率高</td></tr><tr><td rowspan=1 colspan=1>通道数</td></tr></table>

注：参考文档“HYDF 订货号及通讯识别码 V1.0.3”

5.1 校准模块请求被校准模块设置低值
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x85</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x88</td><td rowspan=1 colspan=1>0x88</td></tr></table>

5.2 校准模块请求被校准模块设置高值
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x87</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x88</td><td rowspan=1 colspan=1>0x88</td></tr></table>

5.3 校准模块写入低值实际测得值到被校准模块
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>1 通道数据高</td><td rowspan=1 colspan=1>1 通道数据低</td><td rowspan=1 colspan=1>000</td><td rowspan=1 colspan=1>8通道数据高</td><td rowspan=1 colspan=1>8通道数据低</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x86</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>000</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x88</td><td rowspan=1 colspan=1>0x88</td></tr></table>

5.4 校准模块写入高值实际测得值到被校准模块
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>1 通道数据高</td><td rowspan=1 colspan=1>1 通道数据低</td><td rowspan=1 colspan=1>000</td><td rowspan=1 colspan=1>8通道数据高</td><td rowspan=1 colspan=1>8通道数据低</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td></tr><tr><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>0x88</td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1></td><td rowspan=1 colspan=1>000</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x88</td><td rowspan=1 colspan=1>0x88</td></tr></table>

5.5 校准结束，固化数据到 flash
<table><tr><td rowspan=1 colspan=1>站号</td><td rowspan=1 colspan=1>功能码</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td><td rowspan=1 colspan=1>固定</td></tr><tr><td rowspan=1 colspan=1>0X66</td><td rowspan=1 colspan=1>0x89</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x00</td><td rowspan=1 colspan=1>0x88</td><td rowspan=1 colspan=1>0x88</td></tr></table>

通讯模块数据区地址排列

参考文档 GSDML-V2.35-HYDF-HR\_vxxx-xxxxxxxx 内的通讯模块参数定义

子模块类型：

参阅文档：HYDF 产品内部通讯格式

软元件类型：

0x01：模拟量输入电压

0x02:模拟量输入电流

0x03：IO 输入

0x04：IO 输出

0x05：模拟量输出电压

0x06：模拟量输出电流

0x07：模拟量输入热电阻

0x08：模拟量输入热电偶

0x09：网关通讯数据

0xFE：事务

0xFD：忽略

0XFF:未定义软元件类型

触发事件类型：

0x00：未触发任何改变，不需更新

0x01：未触发任何事件但依然更新软元件

0x02：布尔值改变

0x03：模拟量值改变

0x04：通讯 CRC 错误（事务）

0x05：通讯周期内未收到数据帧（事务）

0x06: 模拟量采集通道溢出（有一个或多个）

0x07：模块内部错误

0x08：通道设置参数 1 为比率

0x09：网关通讯

## 触发事件的元件编号：

1. 子模块所有软原件编号从 0 开始向后排列，有多少资源则排列到多少。10 个IO 输入则编号从0\~9，以此类推,

2.触发类型为布尔值时，如果元件编号为 0xffff，则用字节数据刷新全部 16 个点状态，高位先发(元件新状态 2 字节，16bit)。

## 元件状态：

1.若为电流模拟量，量程 4\~20mA,则发送转换后的数据 4\~20000（低于 3mA 发送 0x7FFF, 表示采集值溢出到了下限,高于 22mA 发送 0x8000，表示采集值溢出到了上限）

2.若为电压模拟量，量程 0\~10V,则发送转换后的数据 0\~10000（高于 10V 发送0x8000 表示采集值溢出到了上限）

3.触发类型为布尔值改变时，如果元件编号为 0Xffff，则用字节数据刷新全部 8 个点状态，高位先发。