# EmEditor JSON 工具 (emjstool)

一个为 [EmEditor](https://www.emeditor.com/) 打造的强大、轻量级的 JSON 处理插件。它不仅支持标准的 JSON 格式化和压缩，还通过 TreeView 视图提供了树状的可视化以及对 JSON5 (带注释、非标语法) 的友好支持。

## 核心功能

- **✨ 智能格式化 (Format)**：一键将杂乱的 JSON 文本整理成整齐可读的格式。使用有序解析模式，确保格式化后键值对顺序不变。
- **🎋 树形视图 (TreeView)**：在侧边栏显示 JSON 结构，支持复杂对象的层级展示。
- **🔍 实时搜索**：在 TreeView 中快速查找键名或文本值，搜索结果支持自动展开与定位。
- **🏗️ JSON5 兼容**：支持处理带有注释、单引号、裸键（Naked Keys）以及尾随逗号的非标准 JSON 文本。
- **🧹 压缩 (Minify)**：去除多余的背景空白和换行。可选保留文件开头的注释信息。
- **🔢 自动排序 (Sort)**：按字母顺序对 JSON 对象的键进行重新排序。
- **🎯 快速定位**：在 TreeView 中点击或双击节点，编辑器将同步定位到源码中的对应位置。
- **⚙️ 高度可配置**：支持自定义缩进宽度（空格或制表符）、换行符模式（CRLF/LF/Auto）以及尾随缩进处理。

## 使用方法

1. **触发菜单**：点击 EmEditor 工具栏上的插件图标，会弹出功能快捷菜单。
2. **执行处理**：
   - **JSFormat**: 格式化当前选中文本或全文。
   - **JSSort**: 排序并格式化。
   - **JSMin**: 压缩 JSON 文本。
3. **管理树视图**：选择 `JSON TreeView` 打开侧边栏容器。
   - 点击侧边栏顶部的 `Refresh` 按钮可同步编辑器中修改后的内容。
   - 在搜索框输入关键词并按回车，可快速定位节点。
4. **调整设置**：通过菜单中的 `Options` 调整插件行为，如 EOL 模式和缩进选项。

## 安装说明

1. 将编译生成的 `jstool.dll` 文件拷贝到 EmEditor 安装目录下的 `Plugins` 目录中。
2. 重启 EmEditor，在 `工具` -> `插件` 菜单中确保 `emjstool` 已被勾选。

## 构建与开发

项目基于 C++ 编写，使用 CMake 作为构建系统。

### 环境依赖

- Visual Studio (推荐 2019/2022)
- Windows SDK

### 构建步骤

```powershell
# 克隆仓库后执行
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## 许可证

本项目采用 [MIT 许可证](LICENSE)。
