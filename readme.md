# CSOL 集成工具

## 功能概述

- **24 H 自定义武器挂机**
- **挂机掉线自动重连**
- **配件自动合成**
- **商店自动重复购买**

## 先决条件

- Windows 10/11
- Logitech G Hub 最新版（无需罗技设备）

## 说明

本项目功能目前均已实现完毕，使用文档也已经发布，详细使用文档在 `releases` 提供的压缩包中，请下载最新版本使用。

<a href="https://gitee.com/silver1867/csol-24-h/releases">点此下载</a>

**本软件使用 GPL v2.0 协议，请尊重作者劳动。**

B 站 ID：_CoreDump

作者邮箱：ttyuig@126.com

为防止二次传播，本项目中的 `Makefile` 、配置文件、LaTeX 文档源码不予提供。原有的 `master`、`dev` 分支已经删除，并替换为唯一的 `main` 分支对外开放。
后续还会将旧版工具代码整理后开源于 `legacy` 分支下。
项目中的 C++ 及 Lua 模块仍保持开源并附以完善注释，供参考交流。

## 功能概览

### 24 H 挂机

支持**单武器挂机**及**随机武器列表**挂机，在网络情况正常时选定角色确保 100% 成功率，支持选定 T/CT 阵营角色。

因网络拥塞、游戏漏洞等问题（维护等特殊情况除外）导致游戏掉线时能自动重连并创建新房间继续挂机。

因诸种因素导致异常退出游戏房间后重新创建房间继续挂机。

### 配件自动合成

制造所自动合成配件功能，需提前输入仓库密码。

### 商店自动重复购买

自动批量购买金币道具，如期限金币角色用于探索升级。