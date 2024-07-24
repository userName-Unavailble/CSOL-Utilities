# CSOL 集成工具

## 功能概述

- **24 H 自定义武器挂机**
- **挂机断线自动重连**
- **配件自动合成**
- **商店自动重复购买**

## 先决条件

- Logitech G Hub 最新版（无需罗技设备）
- Powershell 5 及以上（Windows 8 及以上版本 Windows 自带）

## 说明

本项目功能目前均已实现完毕，使用文档也已经发布，使用文档在下载得到的压缩包中。

<a href="https://gitee.com/silver1867/csol-24-h/releases">点此下载</a>

本集成工具需要搭配 GamingTool 工具使用（原因参见使用文档），具体参考 <a href="https://gitee.com/silver1867/gaming-tool">GamingTool</a> 工具，后续考虑将 GamingTool 工具整合到本项目中。

**本软件使用 GPL v2.0 协议，请尊重作者劳动。**

B 站 ID：_CoreDump

作者邮箱：ttyuig@126.com

## 设计思想

本项目采用 C++ 实现了挂机控制器，控制器通过解析游戏日志文件获取游戏的实时状态，向罗技脚本模块下达对应的挂机命令。
LUA 命令执行模块基于 `dofile` 实现热重载功能，进而实时执行控制器下达的指令。
在设计 LUA 命令执行模块时，本项目首先将罗技 API 封装为更易用的编程接口，且使得罗技脚本能够实时响应外部中断事件，用户可以随时启停。
有关本软件设计的更多细节，敬请关注即将发布的使用文档和开发文档。

## 其他

旧版本的 24H 挂机脚本（无控制器）将在整理后收录于 `legacy` 分支下。`dev` 分支作为频繁更新的分支，软件中存在的任何问题都将在第一时间于 `dev` 分支中更新修复。