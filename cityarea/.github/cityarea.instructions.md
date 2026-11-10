---
description: 'Project-specific coding instructions for cityarea Python GIS processing project'
applyTo: '**/*.py,**/*.md,**/*.json'
---

# cityarea 项目指令

## 范围
- 仅针对当前仓库 `cityarea` 生效。
- 优先修改 `admin_process/`、`city_process/`、`overpass_query/`、`test_read_instruction_md.py` 相关文件。

## 实施原则
- 先检查上下文，再做最小改动实现目标。
- 优先抽取可复用函数，避免一次性脚本逻辑散落。
- 保持中英文注释风格一致，不做无关重构。
- 不提交任何 token、密钥或本机私有配置。

## 验证要求
- 修改行为后，优先运行最小相关脚本/测试验证。
- 如新增配置文件，至少验证文件存在、可读、非空。
